#include "medicion.h"

#include <lorawan.h>
#include <string.h>

#include "config.h"
#include "gps/gps_task.h"
#include "log.h"

Medicion medicion;
uint8_t rcv_count = 0;
char rssiSend[RSSI_SEND_SIZE] = {0};

namespace {

uint8_t sf_en_aire = 0;  // SF del último uplink enviado, todavía sin downlink

// true solo si el payload es "rssi,gateway,contador", por ejemplo "-115,macrointell,15".
// Sin el contador numérico final se descarta. Un comando MAC (0x03) tampoco cumple esto.
bool esMedicionValida(const char *data) {
    if (data == NULL || data[0] != '-') return false;
    const char *p = data + 1;
    if (*p < '0' || *p > '9') return false;
    while (*p >= '0' && *p <= '9') p++;
    if (*p != ',') return false;
    p++;
    while (*p == ' ') p++;
    if (*p == '\0' || *p == ',') return false;
    while (*p != '\0' && *p != ',') p++;
    if (*p != ',') return false;
    p++;
    while (*p == ' ') p++;
    if (*p < '0' || *p > '9') return false;
    while (*p >= '0' && *p <= '9') p++;
    return *p == '\0';
}

void debugDownlinkIgnorado(const char *data, int len) {
#if DEBUG_LOG
    if (len < 0) len = 0;
    if (len > 16) len = 16;
    char hex[16 * 3 + 1] = "";
    for (int i = 0; i < len; i++) {
        snprintf(hex + i * 3, 4, " %02X", static_cast<uint8_t>(data[i]));
    }
    LOG("LORA", "Downlink ignorado, hex:%s", hex);
#else
    (void)data;
    (void)len;
#endif
}

uint8_t sfDeDataRate(unsigned char dr) {
    switch (dr) {
        case SF10BW125: return 10;
        case SF9BW125: return 9;
        case SF8BW125: return 8;
        case SF7BW125: return 7;
        default: return 0;
    }
}

// AU915, canales de 125 kHz: SF7, SF8, SF9, SF10 y vuelve a SF7.
// El data rate de escucha no se toca, para seguir recibiendo el downlink.
void prepararSfDeEnvio() {
    static const uint8_t ciclo[] = {SF7BW125, SF8BW125, SF9BW125, SF10BW125};
    static uint8_t idx = 0;
    lora.setTxDataRate(ciclo[idx]);
    idx = (idx + 1) % (sizeof(ciclo) / sizeof(ciclo[0]));
    sf_en_aire = sfDeDataRate(lora.getDataRate());
    LOG("LORA", "SF envio: %u", sf_en_aire);
}

// Coordenada para el uplink: 6 decimales (resolución que entrega el SIM808).
void formatoCoordenada(bool disponible, double valor, char *out, size_t n) {
    if (disponible) {
        snprintf(out, n, "%.6f", valor);
    } else {
        snprintf(out, n, "0");
    }
}

}  // namespace

bool ProcesarDatoEntrante(char *inputData, int len) {
    if (!esMedicionValida(inputData)) {
        debugDownlinkIgnorado(inputData, len);
        memset(inputData, 0, INPUT_BUFF_SIZE);
        return false;
    }

    // Formato validado: "tx,gateway,seq".
    char *token = strtok(inputData, ",");
    medicion.tx = static_cast<int16_t>(atoi(token));

    token = strtok(NULL, ",");
    strlcpy(medicion.gateway, token, sizeof(medicion.gateway));
    rcv_count++;

    token = strtok(NULL, ",");
    while (*token == ' ') token++;
    medicion.seq = static_cast<uint32_t>(strtoul(token, NULL, 10));

    medicion.rx = static_cast<int16_t>(lora.getRssi());
    medicion.snrRaw = lora.getPktSnrRaw();
    // El TX de este downlink es el RSSI del uplink anterior, enviado con sf_en_aire.
    medicion.sf = sf_en_aire;
    medicion.recibida = true;
    medicion.falla = false;

    char snrTxt[8];
    formatoSnr(medicion.snrRaw, snrTxt, sizeof(snrTxt));
    LOG("LORA", "Downlink OK: tx=%d rx=%d seq=%lu sf=%u snr=%s gateway=%s", medicion.tx, medicion.rx,
        static_cast<unsigned long>(medicion.seq), medicion.sf, snrTxt, medicion.gateway);

    memset(inputData, 0, INPUT_BUFF_SIZE);
    return true;
}

void armarPayloadMedicion() {
    char snrTxt[8];
    formatoSnr(medicion.snrRaw, snrTxt, sizeof(snrTxt));

    double lat = 0;
    double lon = 0;
    const bool gpsOk = gpsGetRecentFix(GPS_MAX_AGE_FOR_UPLINK_MS, lat, lon);
    char latTxt[16];
    char lonTxt[16];
    formatoCoordenada(gpsOk, lat, latTxt, sizeof(latTxt));
    formatoCoordenada(gpsOk, lon, lonTxt, sizeof(lonTxt));
    if (!gpsOk) {
        LOG("LORA", "Sin fix GPS en los ultimos %lu s: lat=0, long=0",
            static_cast<unsigned long>(GPS_MAX_AGE_FOR_UPLINK_MS / 1000));
    }

    snprintf(rssiSend, sizeof(rssiSend), "%d,%d,%lu,%u,%s,%s,%s", medicion.tx, medicion.rx,
             static_cast<unsigned long>(medicion.seq), medicion.sf, snrTxt, latTxt, lonTxt);
    prepararSfDeEnvio();
}

void formatoSnr(int8_t raw, char *out, size_t n) {
    const int cents = static_cast<int>(raw) * 25;
    const int a = abs(cents);
    snprintf(out, n, "%s%d.%02d", cents < 0 ? "-" : "", a / 100, a % 100);
}
