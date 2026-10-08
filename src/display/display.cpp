#include "display.h"

#include <SSD1306Wire.h>
#include <Wire.h>

#include "config.h"
#include "images.h"
#include "log.h"

namespace {

SSD1306Wire oled(OLED_I2C_ADDR, OLED_SDA_PIN, OLED_SCL_PIN, GEOMETRY_128_64);
bool oledOk = false;

// Layout 128x64: fila de 16 px para Tx/Rx y 4 filas de 10 px.
constexpr int16_t ROW_MED_Y = 0;
constexpr int16_t ROW_Y[] = {17, 29, 41, 52};
constexpr int16_t RIGHT_X = 127;

void drawMedicion(const Medicion &med, const char *modoTag, bool pdrOk) {
    char buf[32];
    oled.setFont(ArialMT_Plain_16);

    oled.setTextAlignment(TEXT_ALIGN_LEFT);
    if (med.recibida) {
        snprintf(buf, sizeof(buf), "Tx:%d", med.tx);
    } else {
        snprintf(buf, sizeof(buf), "Tx:--");
    }
    oled.drawString(0, ROW_MED_Y, buf);

    oled.setTextAlignment(TEXT_ALIGN_RIGHT);
    if (med.falla) {
        snprintf(buf, sizeof(buf), "Rx:....");
    } else if (med.recibida) {
        snprintf(buf, sizeof(buf), "Rx:%d", med.rx);
    } else {
        snprintf(buf, sizeof(buf), "Rx:--");
    }
    oled.drawString(RIGHT_X, ROW_MED_Y, buf);

    oled.setFont(ArialMT_Plain_10);
    oled.setTextAlignment(TEXT_ALIGN_RIGHT);
    oled.drawString(RIGHT_X, ROW_Y[1], modoTag);

    oled.setTextAlignment(TEXT_ALIGN_LEFT);
    if (!med.recibida) {
        oled.drawString(0, ROW_Y[0], pdrOk ? "Esperando downlink..." : "Probando red (PDR)...");
        return;
    }
    char snrTxt[8];
    formatoSnr(med.snrRaw, snrTxt, sizeof(snrTxt));
    snprintf(buf, sizeof(buf), "SF%u SNR %s #%lu", med.sf, snrTxt, static_cast<unsigned long>(med.seq));
    oled.drawString(0, ROW_Y[0], buf);
    oled.drawString(0, ROW_Y[1], med.gateway);
}

void drawGps(const GpsStatus &gps) {
    char buf[48];
    const GPSData &d = gps.current;
    oled.setFont(ArialMT_Plain_10);
    oled.setTextAlignment(TEXT_ALIGN_LEFT);

    switch (gps.state) {
        case GpsState::Init:
        case GpsState::Starting:
            oled.drawString(0, ROW_Y[2], "GPS: iniciando...");
            break;

        case GpsState::Searching: {
            oled.drawString(0, ROW_Y[2], "GPS: buscando fix...");
            char vis[6] = "--";
            char uso[6] = "--";
            if (d.satellitesInView != GPS_FIELD_UNAVAILABLE) snprintf(vis, sizeof(vis), "%d", d.satellitesInView);
            if (d.satellites != GPS_FIELD_UNAVAILABLE) snprintf(uso, sizeof(uso), "%d", d.satellites);
            const uint32_t secs = (millis() - gps.searchStartMs) / 1000;
            snprintf(buf, sizeof(buf), "Sat %s vis/%s uso  %lus", vis, uso, static_cast<unsigned long>(secs));
            oled.drawString(0, ROW_Y[3], buf);
            break;
        }

        case GpsState::Fixed:
            snprintf(buf, sizeof(buf), "Lat: %.6f", d.latitude);
            oled.drawString(0, ROW_Y[2], buf);
            snprintf(buf, sizeof(buf), "Lon: %.6f", d.longitude);
            oled.drawString(0, ROW_Y[3], buf);
            oled.setTextAlignment(TEXT_ALIGN_RIGHT);
            if (d.satellites != GPS_FIELD_UNAVAILABLE) {
                snprintf(buf, sizeof(buf), "S:%d", d.satellites);
                oled.drawString(RIGHT_X, ROW_Y[2], buf);
            }
            if (!isnan(d.hdop)) {
                snprintf(buf, sizeof(buf), "H:%.1f", d.hdop);
                oled.drawString(RIGHT_X, ROW_Y[3], buf);
            }
            break;

        case GpsState::Error:
            snprintf(buf, sizeof(buf), "GPS ERROR: %s", gps.errorTitle);
            oled.drawString(0, ROW_Y[2], buf);
            oled.drawString(0, ROW_Y[3], gps.errorDetail);
            break;
    }
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
    oledOk = true;
    LOG("OLED", "SSD1306 128x64 @0x%02X SDA=GPIO%d SCL=GPIO%d", OLED_I2C_ADDR, OLED_SDA_PIN,
        OLED_SCL_PIN);
    return true;
}

void displayDrawMonoImage(int16_t x, int16_t y, const MonoImage &image, bool invert) {
    if (!oledOk || !image.bits) return;
    // drawXbm() de ThingPulse usa el mismo orden de bits que MONO (LSB = pixel izquierdo).
    if (invert) {
        oled.setColor(WHITE);
        oled.fillRect(x, y, image.width, image.height);
        oled.setColor(BLACK);
    }
    oled.drawXbm(x, y, image.width, image.height, image.bits);
    oled.setColor(WHITE);
}

void displayMonoImage(const MonoImage &image, bool invert) {
    if (!oledOk) return;
    oled.clear();
    const int16_t x = (oled.getWidth() - image.width) / 2;
    const int16_t y = (oled.getHeight() - image.height) / 2;
    displayDrawMonoImage(x, y, image, invert);
    oled.display();
}

void displayLogo() {
    displayMonoImage(MonoImage{Logo_width, Logo_height, Logo_bits});
}

void displayStartup() {
    if (!oledOk) return;
    oled.clear();
    oled.setTextAlignment(TEXT_ALIGN_LEFT);
    oled.setFont(ArialMT_Plain_10);
    oled.drawStringMaxWidth(0, 0, 128, FW_VERSION);
    oled.drawStringMaxWidth(0, 40, 128, "Desarrollado por Macro Intell.");
    oled.display();
}

void displayMain(const Medicion &med, const GpsStatus &gps, const char *modoTag, bool pdrOk) {
    if (!oledOk) return;
    oled.clear();
    drawMedicion(med, modoTag, pdrOk);
    drawGps(gps);
    oled.display();
}

void displayInfo(const char *title, const char *line1, const char *line2, const char *line3) {
    if (!oledOk) return;
    oled.clear();
    oled.setTextAlignment(TEXT_ALIGN_LEFT);
    oled.setFont(ArialMT_Plain_16);
    oled.drawString(0, ROW_MED_Y, title);
    oled.drawHorizontalLine(0, 18, 128);
    oled.setFont(ArialMT_Plain_10);
    oled.drawString(0, 22, line1);
    oled.drawString(0, 35, line2);
    oled.drawString(0, 48, line3);
    oled.display();
}

void displayHoldProgress(float fraction, const char *nextMode) {
    if (!oledOk) return;
    const float clamped = fraction < 0 ? 0 : (fraction > 1 ? 1 : fraction);
    oled.clear();
    oled.setTextAlignment(TEXT_ALIGN_LEFT);
    oled.setFont(ArialMT_Plain_16);
    oled.drawString(0, ROW_MED_Y, "Cambiar modo");
    oled.setFont(ArialMT_Plain_10);
    oled.drawString(0, 20, "Siguiente:");
    oled.drawString(0, 32, nextMode);
    oled.drawProgressBar(0, 50, 127, 10, static_cast<uint8_t>(clamped * 100));
    oled.display();
}
