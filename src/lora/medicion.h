#pragma once

#include <Arduino.h>

// Protocolo de medición RSSI (sin cambios respecto de Medidor_RSSI_V2.8, salvo
// que el uplink ahora incluye la posición GPS):
//
//   Uplink:   "tx,rx,seq,sf,snr,lat,long"
//             ej. "-115,-98,15,7,-3.25,-27.468703,-58.829450"
//   Downlink: "tx,gateway,seq" (Node-RED), ej. "-115,macrointell,15"
//
// tx, rx, seq y snr corresponden al downlink anterior. sf es el spreading factor
// con el que salió el uplink que produjo ese tx. lat/long son la última posición
// GPS del SIM808 con antigüedad <= GPS_MAX_AGE_FOR_UPLINK_MS; si no hay, "0,0".

constexpr size_t GATEWAY_NAME_SIZE = 24;
constexpr size_t RSSI_SEND_SIZE = 80;
constexpr size_t INPUT_BUFF_SIZE = 100;

// Resultado de la última medición (último downlink válido).
// Entre paréntesis, el nombre de la variable en Medidor_RSSI_V2.8.
struct Medicion {
    int16_t tx = 0;      // (rssi_rcv)  RSSI con el que el gateway recibió el uplink
    int16_t rx = 0;      // (rssiValue) RSSI con el que el nodo recibió el downlink
    int8_t snrRaw = 0;   // (snr_raw)   SNR del downlink, pasos de 0.25 dB
    uint32_t seq = 0;    // (seq_medicion) secuencia devuelta a Node-RED
    uint8_t sf = 0;      // (sf_medicion)  SF del uplink al que pertenece tx
    char gateway[GATEWAY_NAME_SIZE] = "";  // (get_name)
    bool recibida = false;  // Hubo al menos un downlink válido
    bool falla = false;     // Demasiados envíos sin respuesta (ProcesarFalla)
};

extern Medicion medicion;
extern uint8_t rcv_count;              // Downlinks válidos recibidos
extern char rssiSend[RSSI_SEND_SIZE];  // Último payload de uplink armado

// Procesa un downlink "tx,gateway,seq". Si es válido actualiza `medicion`
// (incluye RX y SNR leídos de la radio) y devuelve true. Limpia `inputData`.
bool ProcesarDatoEntrante(char *inputData, int len);

// Arma rssiSend con la medición actual y la posición GPS, y luego elige el SF
// con el que saldrá este uplink (rota SF7..SF10).
void armarPayloadMedicion();

// SNR dB = raw / 4, con dos decimales (ej. -3.25).
void formatoSnr(int8_t raw, char *out, size_t n);
