#pragma once

#include "gps.h"

// Pantalla OLED integrada del Heltec WiFi LoRa 32 V2 (SSD1306 128x64, I2C).
// Cada función redibuja la pantalla completa.

bool displayInit();
void displayStartup();
void displaySearchingGPS(const GPSData &data, uint32_t searchSeconds);
void displayGPSData(const GPSData &data, uint32_t ageSeconds);
void displayError(const char *title, const char *detail);
