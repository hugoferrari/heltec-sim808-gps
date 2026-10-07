#include "gps.h"

#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "log.h"

namespace {

// Campos de +CGNSINF (índice base 0), según "SIM800 Series GNSS Application
// Note" / manual AT del SIM808:
// <run>,<fix>,<UTC>,<lat>,<lon>,<alt>,<speed>,<course>,<fixMode>,<res1>,
// <HDOP>,<PDOP>,<VDOP>,<res2>,<satsInView>,<satsUsed>,<glonassUsed>,<res3>,
// <C/N0max>,<HPA>,<VPA>
enum CgnsField {
    F_RUN_STATUS = 0,
    F_FIX_STATUS = 1,
    F_LATITUDE = 3,
    F_LONGITUDE = 4,
    F_HDOP = 10,
    F_SATS_IN_VIEW = 14,
    F_SATS_USED = 15,
    CGNS_MIN_FIELDS = 16,
    CGNS_MAX_FIELDS = 24,
};

constexpr size_t AT_RESPONSE_SIZE = 192;

// Divide `str` por comas conservando los campos vacíos. Modifica `str`.
size_t splitFields(char *str, char *fields[], size_t maxFields) {
    size_t count = 0;
    char *p = str;
    while (count < maxFields) {
        fields[count++] = p;
        char *comma = strchr(p, ',');
        if (!comma) {
            break;
        }
        *comma = '\0';
        p = comma + 1;
    }
    return count;
}

// Conversión estricta: el campo debe ser numérico completo y no vacío.
bool parseDouble(const char *s, double &out) {
    if (!s || *s == '\0') {
        return false;
    }
    char *end = nullptr;
    out = strtod(s, &end);
    return end != s && *end == '\0' && isfinite(out);
}

bool parseInt(const char *s, int &out) {
    if (!s || *s == '\0') {
        return false;
    }
    char *end = nullptr;
    const long v = strtol(s, &end, 10);
    if (end == s || *end != '\0') {
        return false;
    }
    out = static_cast<int>(v);
    return true;
}

}  // namespace

const char *gpsReadResultToString(GpsReadResult result) {
    switch (result) {
        case GpsReadResult::Fix: return "FIX";
        case GpsReadResult::NoFix: return "NO_FIX";
        case GpsReadResult::GnssOff: return "GNSS_OFF";
        case GpsReadResult::AtTimeout: return "AT_TIMEOUT";
        case GpsReadResult::AtError: return "AT_ERROR";
        case GpsReadResult::InvalidData: return "INVALID_DATA";
        case GpsReadResult::OutOfRange: return "OUT_OF_RANGE";
    }
    return "?";
}

GpsReadResult parseCgnsInf(const char *response, GPSData &out) {
    out = GPSData{};

    const char *prefix = "+CGNSINF:";
    const char *start = response ? strstr(response, prefix) : nullptr;
    if (!start) {
        return GpsReadResult::InvalidData;
    }
    start += strlen(prefix);
    while (*start == ' ') {
        ++start;
    }

    // Copia local de la línea (hasta '\n') para poder dividirla.
    char line[AT_RESPONSE_SIZE];
    const char *eol = strchr(start, '\n');
    const size_t len = eol ? static_cast<size_t>(eol - start) : strlen(start);
    if (len >= sizeof(line)) {
        return GpsReadResult::InvalidData;
    }
    memcpy(line, start, len);
    line[len] = '\0';

    char *fields[CGNS_MAX_FIELDS];
    const size_t count = splitFields(line, fields, CGNS_MAX_FIELDS);
    if (count < CGNS_MIN_FIELDS) {
        LOG("GPS", "Invalid +CGNSINF: only %u fields", static_cast<unsigned>(count));
        return GpsReadResult::InvalidData;
    }

    int runStatus = 0;
    int fixStatus = 0;
    if (!parseInt(fields[F_RUN_STATUS], runStatus)) {
        return GpsReadResult::InvalidData;
    }
    if (runStatus != 1) {
        return GpsReadResult::GnssOff;
    }

    // Satélites y HDOP pueden estar disponibles aún sin fix.
    int sats = 0;
    if (parseInt(fields[F_SATS_IN_VIEW], sats) && sats >= 0) {
        out.satellitesInView = sats;
    }
    if (parseInt(fields[F_SATS_USED], sats) && sats >= 0) {
        out.satellites = sats;
    }
    double hdop = 0;
    if (parseDouble(fields[F_HDOP], hdop) && hdop >= 0) {
        out.hdop = static_cast<float>(hdop);
    }

    if (!parseInt(fields[F_FIX_STATUS], fixStatus) || fixStatus != 1) {
        return GpsReadResult::NoFix;
    }

    double lat = 0;
    double lon = 0;
    if (!parseDouble(fields[F_LATITUDE], lat) || !parseDouble(fields[F_LONGITUDE], lon)) {
        LOG("GPS", "Fix reported but lat/lon not numeric");
        return GpsReadResult::InvalidData;
    }
    // 0,0 exacto se descarta: en la práctica indica datos basura, no una posición real.
    if (lat < -90.0 || lat > 90.0 || lon < -180.0 || lon > 180.0 || (lat == 0.0 && lon == 0.0)) {
        LOG("GPS", "Coordinates out of range: %.6f, %.6f", lat, lon);
        return GpsReadResult::OutOfRange;
    }

    out.latitude = lat;
    out.longitude = lon;
    out.valid = true;
    return GpsReadResult::Fix;
}

int Sim808Gps::queryPower() {
    char resp[64];
    if (modem_.sendCommand("AT+CGNSPWR?", resp, sizeof(resp)) != AtResult::Ok) {
        return -1;
    }
    const char *p = strstr(resp, "+CGNSPWR:");
    if (!p) {
        LOG("GPS", "Invalid response to AT+CGNSPWR?: '%s'", resp);
        return -1;
    }
    int state = -1;
    if (sscanf(p, "+CGNSPWR: %d", &state) != 1 || (state != 0 && state != 1)) {
        LOG("GPS", "Invalid response to AT+CGNSPWR?: '%s'", resp);
        return -1;
    }
    return state;
}

bool Sim808Gps::start() {
    LOG("GPS", "Starting GPS...");

    const int power = queryPower();
    if (power < 0) {
        // Firmwares antiguos del SIM808 no implementan AT+CGNS* (ver README).
        LOG("GPS", "ERROR: AT+CGNSPWR? not supported or no response");
        return false;
    }
    if (power == 1) {
        LOG("GPS", "GNSS already powered");
    } else if (modem_.sendCommand("AT+CGNSPWR=1", nullptr, 0, AT_GNSS_TIMEOUT_MS) != AtResult::Ok) {
        LOG("GPS", "ERROR: AT+CGNSPWR=1 failed");
        return false;
    }

    if (queryPower() != 1) {
        LOG("GPS", "ERROR: GNSS did not power on");
        return false;
    }
    LOG("GPS", "GNSS powered on. Searching for fix...");
    return true;
}

GpsReadResult Sim808Gps::read(GPSData &out) {
    char resp[AT_RESPONSE_SIZE];
    const AtResult at = modem_.sendCommand("AT+CGNSINF", resp, sizeof(resp), AT_GNSS_TIMEOUT_MS);
    switch (at) {
        case AtResult::Ok: break;
        case AtResult::Timeout:
            out = GPSData{};
            return GpsReadResult::AtTimeout;
        case AtResult::Error:
            out = GPSData{};
            return GpsReadResult::AtError;
        case AtResult::Overflow:
            out = GPSData{};
            return GpsReadResult::InvalidData;
    }
    return parseCgnsInf(resp, out);
}
