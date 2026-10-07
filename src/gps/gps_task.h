#pragma once

#include <Arduino.h>

#include "gps.h"

// Tarea FreeRTOS que maneja el SIM808/GNSS en el core 0.
//
// Los comandos AT son bloqueantes (hasta ~2.5 s), y el ciclo LoRaWAN de Beelan
// también lo es (ventanas RX1/RX2). Separar el GPS en su propia tarea evita que
// uno frene al otro: loop() nunca espera al SIM808 y el GPS sigue
// actualizándose mientras se transmite.
//
// La tarea solo publica su estado; no toca la OLED ni la radio LoRa.

enum class GpsState {
    Init,       // Verificando que el SIM808 responde a AT
    Starting,   // Encendiendo el GNSS
    Searching,  // GNSS encendido, sin fix
    Fixed,      // Fix válido
    Error,      // Error, se reintenta desde Init
};

const char *gpsStateName(GpsState state);

// Copia del estado publicado por la tarea.
struct GpsStatus {
    GpsState state = GpsState::Init;
    GPSData current;                // Última lectura (con o sin fix)
    uint32_t searchStartMs = 0;     // Inicio de la búsqueda de fix actual
    const char *errorTitle = "";    // Solo en GpsState::Error (literales)
    const char *errorDetail = "";
};

// Crea la tarea. Llamar una vez desde setup().
bool gpsTaskStart();

// Estado actual (copia protegida, se puede llamar desde cualquier tarea).
GpsStatus gpsGetStatus();

// Última posición con fix si tiene como máximo `maxAgeMs` de antigüedad.
// Devuelve false si no hay una posición GPS tan reciente.
bool gpsGetRecentFix(uint32_t maxAgeMs, double &latitude, double &longitude);
