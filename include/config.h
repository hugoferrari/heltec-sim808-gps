#pragma once

// =============================================================================
// Configuración central del firmware.
// Todos los pines, tiempos y opciones de depuración se definen aquí.
// =============================================================================

#include <Arduino.h>

// -----------------------------------------------------------------------------
// Depuración por Serial (USB, UART0)
// -----------------------------------------------------------------------------
#define DEBUG_SERIAL_BAUD 115200

// true: muestra por Serial cada comando AT enviado y su respuesta cruda.
// false: solo se muestran los mensajes de diagnóstico de alto nivel.
#define DEBUG_AT true

// -----------------------------------------------------------------------------
// SIM808 - UART2 (HardwareSerial 2)
//
// Pines elegidos para el Heltec WiFi LoRa 32 V2 (ambos expuestos en el header):
//   - GPIO23: libre (no usado por OLED, LoRa, LED ni botón). Usado como RX.
//   - GPIO17: libre. Usado como TX.
// NO se usa el par por defecto de Serial2 (RX=16 / TX=17) porque GPIO16 es el
// reset de la OLED en esta placa.
// -----------------------------------------------------------------------------
constexpr int SIM808_UART_NUM = 2;
constexpr int SIM808_RX_PIN = 23;  // ESP32 RX  <- SIM808 TXD
constexpr int SIM808_TX_PIN = 17;  // ESP32 TX  -> SIM808 RXD
constexpr uint32_t SIM808_BAUD = 9600;  // El SIM808 trae autobauding de fábrica

// Pin opcional para controlar PWRKEY del SIM808 a través de un transistor NPN
// (PWRKEY nunca debe conectarse directamente a un GPIO).
// -1 = deshabilitado (la mayoría de los módulos se encienden con su propio
// botón o tienen PWRKEY cableado). Si se usa, GPIO22 es una opción libre.
constexpr int SIM808_PWRKEY_PIN = -1;
constexpr uint32_t SIM808_PWRKEY_PULSE_MS = 1200;  // Datasheet: >= 1 s en bajo

// -----------------------------------------------------------------------------
// OLED integrada (SSD1306 128x64, I2C)
// -----------------------------------------------------------------------------
constexpr uint8_t OLED_I2C_ADDR = 0x3C;
constexpr int OLED_SDA_PIN = 4;
constexpr int OLED_SCL_PIN = 15;
constexpr int OLED_RST_PIN = 16;
constexpr int VEXT_CTRL_PIN = 21;  // Vext: LOW = alimentación externa 3.3 V activa

// -----------------------------------------------------------------------------
// Tiempos (ms)
// -----------------------------------------------------------------------------
constexpr uint32_t AT_DEFAULT_TIMEOUT_MS = 1000;
constexpr uint32_t AT_GNSS_TIMEOUT_MS = 2000;
constexpr uint8_t AT_SYNC_ATTEMPTS = 5;  // "AT" repetidos para sincronizar autobaud

constexpr uint32_t GPS_POLL_INTERVAL_MS = 2000;      // Consulta AT+CGNSINF
constexpr uint32_t ERROR_RETRY_INTERVAL_MS = 5000;   // Reintento tras un error
constexpr uint8_t MAX_CONSECUTIVE_AT_FAILURES = 3;   // Antes de declarar error
