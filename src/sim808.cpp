#include "sim808.h"

#include <string.h>

#include "config.h"
#include "log.h"

namespace {

constexpr size_t LINE_BUFFER_SIZE = 160;

bool startsWith(const char *str, const char *prefix) {
    return strncmp(str, prefix, strlen(prefix)) == 0;
}

}  // namespace

const char *atResultToString(AtResult result) {
    switch (result) {
        case AtResult::Ok: return "OK";
        case AtResult::Error: return "ERROR";
        case AtResult::Timeout: return "TIMEOUT";
        case AtResult::Overflow: return "OVERFLOW";
    }
    return "?";
}

Sim808::Sim808(HardwareSerial &serial, int rxPin, int txPin, uint32_t baud, int pwrKeyPin)
    : serial_(serial), rxPin_(rxPin), txPin_(txPin), baud_(baud), pwrKeyPin_(pwrKeyPin) {}

void Sim808::begin() {
    serial_.begin(baud_, SERIAL_8N1, rxPin_, txPin_);
    if (pwrKeyPin_ >= 0) {
        // El GPIO maneja la base de un NPN: LOW = PWRKEY liberado.
        pinMode(pwrKeyPin_, OUTPUT);
        digitalWrite(pwrKeyPin_, LOW);
    }
    LOG("SIM808", "UART%d RX=GPIO%d TX=GPIO%d @ %lu baud", SIM808_UART_NUM, rxPin_, txPin_,
        static_cast<unsigned long>(baud_));
}

bool Sim808::checkAlive() {
    LOG("SIM808", "Initializing...");
    for (uint8_t attempt = 1; attempt <= AT_SYNC_ATTEMPTS; ++attempt) {
        if (sendCommand("AT", nullptr, 0, 500) == AtResult::Ok) {
            LOG("SIM808", "AT OK");
            // Sin eco las respuestas son más simples de procesar.
            if (sendCommand("ATE0") != AtResult::Ok) {
                LOG("SIM808", "WARN: ATE0 failed");
            }
            return true;
        }
    }
    LOG("SIM808", "ERROR: no response to AT after %u attempts", AT_SYNC_ATTEMPTS);
    return false;
}

AtResult Sim808::sendCommand(const char *command, char *response, size_t responseSize,
                             uint32_t timeoutMs) {
    if (response && responseSize > 0) {
        response[0] = '\0';
    }
    clearInput();

    LOG_AT(">> %s", command);
    serial_.print(command);
    serial_.print("\r");

    char line[LINE_BUFFER_SIZE];
    size_t lineLen = 0;
    size_t respLen = 0;
    bool overflow = false;
    const uint32_t start = millis();

    while (millis() - start < timeoutMs) {
        if (!serial_.available()) {
            delay(1);  // Cede CPU a otras tareas (watchdog / WiFi en el futuro)
            continue;
        }

        const char c = static_cast<char>(serial_.read());
        if (c == '\r') {
            continue;
        }
        if (c != '\n') {
            if (lineLen < LINE_BUFFER_SIZE - 1) {
                line[lineLen++] = c;
            } else {
                overflow = true;
            }
            continue;
        }

        // Línea completa.
        line[lineLen] = '\0';
        lineLen = 0;
        if (line[0] == '\0' || strcmp(line, command) == 0) {
            continue;  // Línea vacía o eco del comando
        }
        LOG_AT("<< %s", line);

        if (strcmp(line, "OK") == 0) {
            return overflow ? AtResult::Overflow : AtResult::Ok;
        }
        if (strcmp(line, "ERROR") == 0 || startsWith(line, "+CME ERROR") ||
            startsWith(line, "+CMS ERROR")) {
            LOG("SIM808", "ERROR response to '%s': %s", command, line);
            return AtResult::Error;
        }

        // Línea de información: se agrega a la respuesta.
        if (response && responseSize > 0) {
            const size_t len = strlen(line);
            const size_t needed = len + (respLen > 0 ? 1 : 0);
            if (respLen + needed < responseSize) {
                if (respLen > 0) {
                    response[respLen++] = '\n';
                }
                memcpy(response + respLen, line, len);
                respLen += len;
                response[respLen] = '\0';
            } else {
                overflow = true;
            }
        }
    }

    LOG("SIM808", "TIMEOUT (%lu ms) waiting response to '%s'",
        static_cast<unsigned long>(timeoutMs), command);
    return AtResult::Timeout;
}

void Sim808::clearInput() {
#if DEBUG_AT
    size_t discarded = 0;
#endif
    while (serial_.available()) {
        serial_.read();
#if DEBUG_AT
        ++discarded;
#endif
    }
#if DEBUG_AT
    if (discarded > 0) {
        LOG_AT("(discarded %u stale bytes)", static_cast<unsigned>(discarded));
    }
#endif
}

bool Sim808::pulsePowerKey() {
    if (pwrKeyPin_ < 0) {
        return false;
    }
    LOG("SIM808", "Pulsing PWRKEY (GPIO%d) for %lu ms", pwrKeyPin_,
        static_cast<unsigned long>(SIM808_PWRKEY_PULSE_MS));
    digitalWrite(pwrKeyPin_, HIGH);  // NPN conduce -> PWRKEY a GND
    delay(SIM808_PWRKEY_PULSE_MS);
    digitalWrite(pwrKeyPin_, LOW);
    return true;
}
