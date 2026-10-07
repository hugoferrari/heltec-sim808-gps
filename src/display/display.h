#pragma once

#include "gps/gps_task.h"
#include "lora/medicion.h"

// OLED integrada del Heltec WiFi LoRa 32 V2 (SSD1306 128x64, I2C).
// Solo se usa desde loop() (core 1); la tarea GPS no dibuja.
//
// Pantalla principal:
//   Tx:-115          Rx:-98     <- medición (fuente 16)
//   SF7 SNR -3.25 #15
//   macrointell                 <- gateway
//   Lat: -27.468703      S:8    <- GPS (o estado: buscando / error)
//   Lon: -58.829450    H:1.1

bool displayInit();
void displayStartup();
void displayMain(const Medicion &med, const GpsStatus &gps);

// Redibuja la pantalla principal con el estado actual de medición y GPS.
void displayRefresh();
