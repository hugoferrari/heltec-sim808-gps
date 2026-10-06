// Heltec WiFi LoRa 32 V2 + SIM808: lectura de posición GNSS y visualización en OLED.
//
// main.cpp solo contiene la máquina de estados de la aplicación; la comunicación
// AT (sim808), el GNSS (gps) y la pantalla (display) viven en módulos separados.

#include <Arduino.h>

#include "config.h"
#include "display.h"
#include "gps.h"
#include "log.h"
#include "sim808.h"

namespace {

enum class AppState {
    Init,          // Verificar que el SIM808 responde a AT
    GpsStart,      // Encender el GNSS
    GpsSearching,  // GNSS encendido, esperando fix
    GpsFixed,      // Fix válido, posición actualizada periódicamente
    GpsError,      // Error: se muestra y se reintenta desde Init
};

const char *stateName(AppState s) {
    switch (s) {
        case AppState::Init: return "INIT";
        case AppState::GpsStart: return "GPS_START";
        case AppState::GpsSearching: return "GPS_SEARCHING";
        case AppState::GpsFixed: return "GPS_FIXED";
        case AppState::GpsError: return "GPS_ERROR";
    }
    return "?";
}

HardwareSerial sim808Serial(SIM808_UART_NUM);
Sim808 modem(sim808Serial, SIM808_RX_PIN, SIM808_TX_PIN, SIM808_BAUD, SIM808_PWRKEY_PIN);
Sim808Gps gps(modem);

AppState state = AppState::Init;
uint32_t stateSinceMs = 0;
uint32_t lastPollMs = 0;
uint32_t searchStartMs = 0;
uint32_t lastFixMs = 0;
uint8_t atFailures = 0;    // Fallos AT consecutivos durante la consulta GNSS
uint8_t initFailures = 0;  // Intentos fallidos de detectar el SIM808
GPSData gpsData;           // Última lectura (con fix o no)

void enterState(AppState next) {
    if (next != state) {
        LOG("APP", "State %s -> %s", stateName(state), stateName(next));
    }
    state = next;
    stateSinceMs = millis();
}

void enterError(const char *title, const char *detail) {
    LOG("APP", "ERROR: %s - %s (retry in %lu ms)", title, detail,
        static_cast<unsigned long>(ERROR_RETRY_INTERVAL_MS));
    displayError(title, detail);
    enterState(AppState::GpsError);
}

uint32_t secondsSince(uint32_t ms) {
    return (millis() - ms) / 1000;
}

void handleInit() {
    if (modem.checkAlive()) {
        initFailures = 0;
        atFailures = 0;
        enterState(AppState::GpsStart);
        return;
    }
    // Si hay PWRKEY cableado, intentar encender el módulo tras dos fallos seguidos.
    if (++initFailures >= 2 && modem.pulsePowerKey()) {
        initFailures = 0;
    }
    enterError("SIM808", "No responde");
}

void handleGpsStart() {
    if (!gps.start()) {
        enterError("GPS", "No inicia");
        return;
    }
    gpsData = GPSData{};
    searchStartMs = millis();
    lastPollMs = millis() - GPS_POLL_INTERVAL_MS;  // Consultar de inmediato
    displaySearchingGPS(gpsData, 0);
    enterState(AppState::GpsSearching);
}

void onFix(const GPSData &sample) {
    gpsData = sample;
    lastFixMs = millis();
    if (state != AppState::GpsFixed) {
        LOG("GPS", "FIX acquired after %lu s", static_cast<unsigned long>(secondsSince(searchStartMs)));
        enterState(AppState::GpsFixed);
    }
    LOG("GPS", "Latitude: %.6f", gpsData.latitude);
    LOG("GPS", "Longitude: %.6f", gpsData.longitude);
    LOG("GPS", "Satellites: %d  HDOP: %.1f", gpsData.satellites, gpsData.hdop);
    displayGPSData(gpsData, 0);
}

void onNoFix(const GPSData &sample) {
    if (state == AppState::GpsFixed) {
        LOG("GPS", "Fix lost, searching again");
        searchStartMs = millis();
        enterState(AppState::GpsSearching);
    }
    gpsData = sample;
    LOG("GPS", "Searching for fix... (%lu s, sats in view: %d, used: %d)",
        static_cast<unsigned long>(secondsSince(searchStartMs)), gpsData.satellitesInView,
        gpsData.satellites);
    displaySearchingGPS(gpsData, secondsSince(searchStartMs));
}

void onAtFailure(GpsReadResult result) {
    ++atFailures;
    LOG("GPS", "Read failed: %s (%u/%u)", gpsReadResultToString(result), atFailures,
        MAX_CONSECUTIVE_AT_FAILURES);
    if (atFailures >= MAX_CONSECUTIVE_AT_FAILURES) {
        enterError("SIM808", "Sin respuesta AT");
        return;
    }
    // Fallo transitorio: mantener en pantalla la última información conocida.
    if (state == AppState::GpsFixed) {
        displayGPSData(gpsData, secondsSince(lastFixMs));
    } else {
        displaySearchingGPS(gpsData, secondsSince(searchStartMs));
    }
}

void handleGpsPolling() {
    if (millis() - lastPollMs < GPS_POLL_INTERVAL_MS) {
        return;
    }
    lastPollMs = millis();

    GPSData sample;
    const GpsReadResult result = gps.read(sample);
    switch (result) {
        case GpsReadResult::Fix:
            atFailures = 0;
            onFix(sample);
            break;
        case GpsReadResult::InvalidData:
        case GpsReadResult::OutOfRange:
            LOG("GPS", "Discarding invalid data (%s)", gpsReadResultToString(result));
            [[fallthrough]];
        case GpsReadResult::NoFix:
            atFailures = 0;
            onNoFix(sample);
            break;
        case GpsReadResult::GnssOff:
            // Puede ocurrir si el SIM808 se reinició (p. ej. caída de tensión).
            LOG("GPS", "GNSS reported OFF, restarting it");
            atFailures = 0;
            enterState(AppState::GpsStart);
            break;
        case GpsReadResult::AtTimeout:
        case GpsReadResult::AtError:
            onAtFailure(result);
            break;
    }
}

void handleError() {
    if (millis() - stateSinceMs >= ERROR_RETRY_INTERVAL_MS) {
        enterState(AppState::Init);
    }
}

}  // namespace

void setup() {
    Serial.begin(DEBUG_SERIAL_BAUD);
    delay(200);  // Dar tiempo al monitor serie USB
    LOG("APP", "Heltec WiFi LoRa 32 V2 + SIM808 GPS");

    if (!displayInit()) {
        LOG("APP", "Continuing without OLED");
    }
    displayStartup();

    modem.begin();
    enterState(AppState::Init);
}

void loop() {
    switch (state) {
        case AppState::Init: handleInit(); break;
        case AppState::GpsStart: handleGpsStart(); break;
        case AppState::GpsSearching:
        case AppState::GpsFixed: handleGpsPolling(); break;
        case AppState::GpsError: handleError(); break;
    }
}
