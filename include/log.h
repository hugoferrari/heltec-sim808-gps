#pragma once

#include <Arduino.h>

#include "config.h"

// Mensajes de diagnóstico de alto nivel: siempre activos.
#define LOG(tag, fmt, ...) Serial.printf("[" tag "] " fmt "\n", ##__VA_ARGS__)

// Tráfico AT crudo: se compila solo si DEBUG_AT es true.
#if DEBUG_AT
#define LOG_AT(fmt, ...) Serial.printf("[AT] " fmt "\n", ##__VA_ARGS__)
#else
#define LOG_AT(fmt, ...) \
    do {                 \
    } while (0)
#endif
