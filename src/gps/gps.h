#pragma once

#include <Arduino.h>
#include <math.h>

#include "sim808.h"

// Valor usado para campos enteros no disponibles.
constexpr int GPS_FIELD_UNAVAILABLE = -1;

// Posición GPS obtenida del SIM808.
// Campos no disponibles: NAN (double/float) o GPS_FIELD_UNAVAILABLE (int).
struct GPSData {
    bool valid = false;            // true solo con fix y coordenadas validadas
    double latitude = NAN;         // grados decimales, -90..90 (WGS84)
    double longitude = NAN;        // grados decimales, -180..180 (WGS84)
    int satellites = GPS_FIELD_UNAVAILABLE;        // satélites usados en el fix
    int satellitesInView = GPS_FIELD_UNAVAILABLE;  // satélites visibles
    float hdop = NAN;
};

// Resultado de una consulta de posición.
enum class GpsReadResult {
    Fix,           // Fix válido, coordenadas dentro de rango
    NoFix,         // GNSS encendido pero todavía sin fix
    GnssOff,       // El SIM808 reporta el GNSS apagado
    AtTimeout,     // El SIM808 no respondió
    AtError,       // El SIM808 respondió ERROR
    InvalidData,   // Respuesta con formato inesperado o campos no numéricos
    OutOfRange,    // Fix reportado pero coordenadas fuera de rango
};

const char *gpsReadResultToString(GpsReadResult result);

// Interpreta la línea "+CGNSINF: ..." (función pura, sin acceso a hardware).
GpsReadResult parseCgnsInf(const char *response, GPSData &out);

// GNSS del SIM808 mediante los comandos AT+CGNS* (firmware SIM808 R14 o posterior).
class Sim808Gps {
public:
    explicit Sim808Gps(Sim808 &modem) : modem_(modem) {}

    // Enciende el GNSS (AT+CGNSPWR=1) si no lo estaba y verifica el estado.
    bool start();

    // Consulta AT+CGNSINF y completa `out`.
    GpsReadResult read(GPSData &out);

private:
    // Devuelve 1 encendido, 0 apagado, -1 sin respuesta válida.
    int queryPower();

    Sim808 &modem_;
};
