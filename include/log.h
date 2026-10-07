#pragma once

#include <Arduino.h>

#include "config.h"

// Mensajes de diagnóstico de alto nivel ("[TAG] mensaje").
// Serial es seguro entre tareas en Arduino-ESP32 (cada printf toma un lock).
#if DEBUG_LOG
#define LOG(tag, fmt, ...) Serial.printf("[" tag "] " fmt "\n", ##__VA_ARGS__)
#else
#define LOG(tag, fmt, ...) \
    do {                   \
    } while (0)
#endif

// Tráfico AT crudo del SIM808.
#if DEBUG_AT
#define LOG_AT(fmt, ...) Serial.printf("[AT] " fmt "\n", ##__VA_ARGS__)
#else
#define LOG_AT(fmt, ...) \
    do {                 \
    } while (0)
#endif
