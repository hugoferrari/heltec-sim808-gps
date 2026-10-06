#pragma once

#include <Arduino.h>
#include <HardwareSerial.h>

#include "config.h"

// Resultado de un comando AT.
enum class AtResult {
    Ok,        // El módem respondió "OK"
    Error,     // "ERROR", "+CME ERROR: ..." o "+CMS ERROR: ..."
    Timeout,   // No llegó una respuesta final dentro del timeout
    Overflow,  // La respuesta no cupo en el buffer provisto
};

const char *atResultToString(AtResult result);

// Capa mínima de comunicación AT con el SIM808 sobre HardwareSerial.
// Es independiente del GPS para poder reutilizarla luego en GSM/GPRS.
class Sim808 {
public:
    Sim808(HardwareSerial &serial, int rxPin, int txPin, uint32_t baud, int pwrKeyPin);

    void begin();

    // Envía "AT" varias veces (sincroniza el autobaud) y desactiva el eco.
    // Devuelve true si el módem responde.
    bool checkAlive();

    // Envía un comando AT y espera la respuesta final ("OK"/"ERROR").
    // Las líneas intermedias (p. ej. "+CGNSINF: ...") se copian en `response`
    // separadas por '\n'. `response` puede ser nullptr si no interesa.
    AtResult sendCommand(const char *command, char *response = nullptr,
                         size_t responseSize = 0,
                         uint32_t timeoutMs = AT_DEFAULT_TIMEOUT_MS);

    // Descarta cualquier byte pendiente en el buffer de recepción UART.
    void clearInput();

    // Pulso en PWRKEY (solo si hay pin configurado). Bloquea ~1.2 s.
    // OJO: el mismo pulso enciende o apaga el módulo según su estado actual.
    bool pulsePowerKey();

private:
    HardwareSerial &serial_;
    const int rxPin_;
    const int txPin_;
    const uint32_t baud_;
    const int pwrKeyPin_;
};
