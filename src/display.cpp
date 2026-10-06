#include "display.h"

#include <SSD1306Wire.h>
#include <Wire.h>

#include "config.h"
#include "log.h"

namespace {

SSD1306Wire oled(OLED_I2C_ADDR, OLED_SDA_PIN, OLED_SCL_PIN, GEOMETRY_128_64);

// Layout para 128x64: título con fuente 16 px y 4 líneas de 10 px (12 px de paso).
constexpr int16_t LINE_Y[] = {18, 30, 42, 54};

void drawHeader(const char *title) {
    oled.clear();
    oled.setTextAlignment(TEXT_ALIGN_LEFT);
    oled.setFont(ArialMT_Plain_16);
    oled.drawString(0, 0, title);
    oled.setFont(ArialMT_Plain_10);
}

void drawLine(uint8_t index, const char *text) {
    oled.drawString(0, LINE_Y[index], text);
}

}  // namespace

bool displayInit() {
    // Vext alimenta periféricos externos de 3.3 V; activarlo no afecta a la OLED
    // si esta se alimenta directamente, y la deja operativa si depende de Vext.
    pinMode(VEXT_CTRL_PIN, OUTPUT);
    digitalWrite(VEXT_CTRL_PIN, LOW);

    // La OLED del Heltec V2 requiere un pulso de reset en GPIO16.
    pinMode(OLED_RST_PIN, OUTPUT);
    digitalWrite(OLED_RST_PIN, LOW);
    delay(20);
    digitalWrite(OLED_RST_PIN, HIGH);
    delay(20);

    if (!oled.init()) {
        LOG("OLED", "ERROR: init failed");
        return false;
    }
    oled.flipScreenVertically();
    oled.setContrast(255);
    LOG("OLED", "SSD1306 128x64 @0x%02X SDA=GPIO%d SCL=GPIO%d", OLED_I2C_ADDR, OLED_SDA_PIN,
        OLED_SCL_PIN);
    return true;
}

void displayStartup() {
    drawHeader("Heltec SIM808");
    drawLine(0, "GPS tracker");
    drawLine(1, "Iniciando SIM808...");
    oled.display();
}

void displaySearchingGPS(const GPSData &data, uint32_t searchSeconds) {
    char buf[32];
    drawHeader("GPS");
    drawLine(0, "Buscando fix...");

    if (data.satellitesInView != GPS_FIELD_UNAVAILABLE) {
        snprintf(buf, sizeof(buf), "Sat visibles: %d", data.satellitesInView);
    } else {
        snprintf(buf, sizeof(buf), "Sat visibles: --");
    }
    drawLine(1, buf);

    if (data.satellites != GPS_FIELD_UNAVAILABLE) {
        snprintf(buf, sizeof(buf), "Sat usados: %d", data.satellites);
    } else {
        snprintf(buf, sizeof(buf), "Sat usados: --");
    }
    drawLine(2, buf);

    snprintf(buf, sizeof(buf), "Tiempo: %lu s", static_cast<unsigned long>(searchSeconds));
    drawLine(3, buf);
    oled.display();
}

void displayGPSData(const GPSData &data, uint32_t ageSeconds) {
    char buf[32];
    drawHeader("GPS FIX");

    snprintf(buf, sizeof(buf), "Lat: %.6f", data.latitude);
    drawLine(0, buf);
    snprintf(buf, sizeof(buf), "Lon: %.6f", data.longitude);
    drawLine(1, buf);

    char sats[8] = "--";
    char hdop[8] = "--";
    if (data.satellites != GPS_FIELD_UNAVAILABLE) {
        snprintf(sats, sizeof(sats), "%d", data.satellites);
    }
    if (!isnan(data.hdop)) {
        snprintf(hdop, sizeof(hdop), "%.1f", data.hdop);
    }
    snprintf(buf, sizeof(buf), "Sat: %s  HDOP: %s", sats, hdop);
    drawLine(2, buf);

    snprintf(buf, sizeof(buf), "Actualizado: %lu s", static_cast<unsigned long>(ageSeconds));
    drawLine(3, buf);
    oled.display();
}

void displayError(const char *title, const char *detail) {
    drawHeader("ERROR");
    drawLine(0, title);
    drawLine(1, detail);
    drawLine(3, "Reintentando...");
    oled.display();
}
