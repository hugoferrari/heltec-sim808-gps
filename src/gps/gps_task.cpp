#include "gps_task.h"

#include "config.h"
#include "log.h"
#include "sim808.h"

namespace {

HardwareSerial sim808Serial(SIM808_UART_NUM);
Sim808 modem(sim808Serial, SIM808_RX_PIN, SIM808_TX_PIN, SIM808_BAUD, SIM808_PWRKEY_PIN);
Sim808Gps gps(modem);

// Estado compartido con otras tareas, protegido por `lock`.
portMUX_TYPE lock = portMUX_INITIALIZER_UNLOCKED;
GpsStatus shared;
GPSData lastFix;           // Última posición con fix
uint32_t lastFixMs = 0;
bool hasFix = false;

// Estado local de la máquina (solo lo usa la tarea GPS).
GpsStatus local;
uint32_t stateSinceMs = 0;
uint32_t lastPollMs = 0;
uint8_t atFailures = 0;
uint8_t initFailures = 0;

void publish() {
    portENTER_CRITICAL(&lock);
    shared = local;
    portEXIT_CRITICAL(&lock);
}

void publishFix(const GPSData &fix) {
    const uint32_t now = millis();
    portENTER_CRITICAL(&lock);
    lastFix = fix;
    lastFixMs = now;
    hasFix = true;
    portEXIT_CRITICAL(&lock);
}

void enterState(GpsState next) {
    if (next != local.state) {
        LOG("GPS", "State %s -> %s", gpsStateName(local.state), gpsStateName(next));
    }
    local.state = next;
    stateSinceMs = millis();
    publish();
}

void enterError(const char *title, const char *detail) {
    LOG("GPS", "ERROR: %s - %s (retry in %lu ms)", title, detail,
        static_cast<unsigned long>(ERROR_RETRY_INTERVAL_MS));
    local.errorTitle = title;
    local.errorDetail = detail;
    enterState(GpsState::Error);
}

uint32_t secondsSince(uint32_t ms) {
    return (millis() - ms) / 1000;
}

void handleInit() {
    if (modem.checkAlive()) {
        initFailures = 0;
        atFailures = 0;
        enterState(GpsState::Starting);
        return;
    }
    // Si hay PWRKEY cableado, intentar encender el módulo tras dos fallos seguidos.
    if (++initFailures >= 2 && modem.pulsePowerKey()) {
        initFailures = 0;
    }
    enterError("SIM808", "No responde");
}

void handleStarting() {
    if (!gps.start()) {
        enterError("GPS", "No inicia");
        return;
    }
    local.current = GPSData{};
    local.searchStartMs = millis();
    lastPollMs = millis() - GPS_POLL_INTERVAL_MS;  // Consultar de inmediato
    enterState(GpsState::Searching);
}

void onFix(const GPSData &sample) {
    local.current = sample;
    publishFix(sample);
    if (local.state != GpsState::Fixed) {
        LOG("GPS", "FIX acquired after %lu s", static_cast<unsigned long>(secondsSince(local.searchStartMs)));
        enterState(GpsState::Fixed);
    } else {
        publish();
    }
    LOG("GPS", "Lat: %.6f  Lon: %.6f  Sat: %d  HDOP: %.1f", sample.latitude, sample.longitude,
        sample.satellites, sample.hdop);
}

void onNoFix(const GPSData &sample) {
    local.current = sample;
    if (local.state == GpsState::Fixed) {
        LOG("GPS", "Fix lost, searching again");
        local.searchStartMs = millis();
        enterState(GpsState::Searching);
    } else {
        publish();
    }
    LOG("GPS", "Searching for fix... (%lu s, sats in view: %d, used: %d)",
        static_cast<unsigned long>(secondsSince(local.searchStartMs)), sample.satellitesInView,
        sample.satellites);
}

void onAtFailure(GpsReadResult result) {
    ++atFailures;
    LOG("GPS", "Read failed: %s (%u/%u)", gpsReadResultToString(result), atFailures,
        MAX_CONSECUTIVE_AT_FAILURES);
    if (atFailures >= MAX_CONSECUTIVE_AT_FAILURES) {
        enterError("SIM808", "Sin respuesta AT");
    }
}

void handlePolling() {
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
            enterState(GpsState::Starting);
            break;
        case GpsReadResult::AtTimeout:
        case GpsReadResult::AtError:
            onAtFailure(result);
            break;
    }
}

void handleError() {
    if (millis() - stateSinceMs >= ERROR_RETRY_INTERVAL_MS) {
        enterState(GpsState::Init);
    }
}

void gpsTask(void *) {
    modem.begin();
    for (;;) {
        switch (local.state) {
            case GpsState::Init: handleInit(); break;
            case GpsState::Starting: handleStarting(); break;
            case GpsState::Searching:
            case GpsState::Fixed: handlePolling(); break;
            case GpsState::Error: handleError(); break;
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

}  // namespace

const char *gpsStateName(GpsState state) {
    switch (state) {
        case GpsState::Init: return "INIT";
        case GpsState::Starting: return "GPS_START";
        case GpsState::Searching: return "GPS_SEARCHING";
        case GpsState::Fixed: return "GPS_FIXED";
        case GpsState::Error: return "GPS_ERROR";
    }
    return "?";
}

bool gpsTaskStart() {
    const BaseType_t ok = xTaskCreatePinnedToCore(gpsTask, "gps", GPS_TASK_STACK_BYTES, nullptr,
                                                  GPS_TASK_PRIORITY, nullptr, GPS_TASK_CORE);
    if (ok != pdPASS) {
        LOG("GPS", "ERROR: could not create GPS task");
        return false;
    }
    return true;
}

GpsStatus gpsGetStatus() {
    portENTER_CRITICAL(&lock);
    const GpsStatus copy = shared;
    portEXIT_CRITICAL(&lock);
    return copy;
}

bool gpsGetRecentFix(uint32_t maxAgeMs, double &latitude, double &longitude) {
    const uint32_t now = millis();
    portENTER_CRITICAL(&lock);
    const bool fresh = hasFix && (now - lastFixMs) <= maxAgeMs;
    if (fresh) {
        latitude = lastFix.latitude;
        longitude = lastFix.longitude;
    }
    portEXIT_CRITICAL(&lock);
    return fresh;
}
