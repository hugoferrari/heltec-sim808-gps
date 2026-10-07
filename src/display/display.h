#pragma once

#include "gps/gps_task.h"
#include "lora/medicion.h"

// OLED integrada del Heltec WiFi LoRa 32 V2 (SSD1306 128x64, I2C).
// Solo dibuja; qué pantalla mostrar y cuándo lo decide ui/ui.h.
// Se usa solo desde loop() (core 1).
//
// Pantalla principal:
//   Tx:-115          Rx:-98     <- medición (fuente 16)
//   SF7 SNR -3.25 #15
//   macrointell       AUTO 10s  <- gateway y modo
//   Lat: -27.468703      S:8    <- GPS (o estado: buscando / error)
//   Lon: -58.829450    H:1.1

bool displayInit();
void displayStartup();

// `modoTag`: modo abreviado ("PRG", "AUTO 10s", "OFF").
// `pdrOk`: false muestra "Probando red (PDR)..." hasta el primer downlink.
void displayMain(const Medicion &med, const GpsStatus &gps, const char *modoTag, bool pdrOk);

// Pantalla de aviso: título (fuente 16) y hasta 3 líneas (fuente 10, pueden ser "").
void displayInfo(const char *title, const char *line1, const char *line2, const char *line3);

// Progreso de la pulsación larga de PRG (0.0 a 1.0) y modo al que se pasará.
void displayHoldProgress(float fraction, const char *nextMode);
