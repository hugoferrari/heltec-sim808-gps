#pragma once

// =============================================================================
// Configuración central del firmware.
// Pines, tiempos y opciones de depuración. Las credenciales LoRaWAN están en
// include/secrets.h (no versionado; ver include/secrets.example.h).
// =============================================================================

#include <Arduino.h>

#define FW_VERSION "Medidor RSSI V3.1 GPS"

// -----------------------------------------------------------------------------
// Depuración por Serial (USB, UART0)
// -----------------------------------------------------------------------------
#define DEBUG_SERIAL_BAUD 115200

// true: mensajes de diagnóstico de todos los módulos ([LORA], [GPS], ...).
#define DEBUG_LOG true

// true: muestra cada comando AT enviado al SIM808 y su respuesta cruda.
#define DEBUG_AT false

// -----------------------------------------------------------------------------
// LoRa SX1276 integrado (pines fijos del Heltec WiFi LoRa 32 V2)
// SPI: SCK=5, MISO=19, MOSI=27 (definidos por el variant de la placa).
// -----------------------------------------------------------------------------
constexpr int LORA_CS_PIN = 18;
constexpr int LORA_RST_PIN = 14;
constexpr int LORA_DIO0_PIN = 26;
constexpr int LORA_DIO1_PIN = 35;  // Cableado internamente en la V2
constexpr int LORA_DIO2_PIN = 34;  // Cableado internamente en la V2

// -----------------------------------------------------------------------------
// Esquema de medición / transmisión LoRaWAN
// -----------------------------------------------------------------------------
constexpr uint8_t LORA_PORT = 1;
constexpr uint32_t ACK_POLL_INTERVAL_MS = 500;  // Consulta de ACK del PDR
constexpr uint16_t ACK_TIMEOUT_TICKS = 150;     // 150 x 500 ms = 75 s sin ACK
constexpr long PDR_RETRY_MIN_S = 5;             // Espera aleatoria entre
constexpr long PDR_RETRY_MAX_S = 10;            // reintentos del PDR (s)
constexpr uint8_t MAX_DIFERENCIA_ENVIOS = 3;    // Envíos sin respuesta -> falla

// Intervalos del modo "Envío automático", en el orden en que se recorren con
// doble pulsación de PRG. El primero es el valor por defecto.
constexpr uint32_t ENVIO_INTERVALOS_MS[] = {10000, 30000, 60000};

// Antigüedad máxima de una posición GPS para incluirla en el uplink.
// Si no hay fix más reciente se envía lat=0, long=0.
constexpr uint32_t GPS_MAX_AGE_FOR_UPLINK_MS = 30000;

// -----------------------------------------------------------------------------
// SIM808 - UART2 (HardwareSerial 2)
//
// GPIO23 y GPIO17 están libres en el Heltec V2 (no los usan OLED, LoRa, LED ni
// botón). No se usa el par por defecto de Serial2 (RX=16) porque GPIO16 es el
// reset de la OLED.
// -----------------------------------------------------------------------------
constexpr int SIM808_UART_NUM = 2;
constexpr int SIM808_RX_PIN = 23;       // ESP32 RX  <- SIM808 TXD
constexpr int SIM808_TX_PIN = 17;       // ESP32 TX  -> SIM808 RXD
constexpr uint32_t SIM808_BAUD = 9600;  // El SIM808 trae autobauding de fábrica

// Pin opcional para PWRKEY a través de un transistor NPN (nunca directo).
// -1 = deshabilitado. Si se usa, GPIO22 es una opción libre.
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

constexpr uint32_t DISPLAY_REFRESH_MS = 1000;      // Pantalla principal
constexpr uint32_t DISPLAY_FAST_REFRESH_MS = 100;  // Barra de progreso al mantener PRG
constexpr uint32_t UI_OVERLAY_MS = 2500;           // Avisos (modo, intervalo, envío)
constexpr uint32_t SPLASH_LOGO_MS = 2500;      // Pantalla de bienvenida con el logo
constexpr uint32_t SPLASH_DURATION_MS = 1500;  // Pantalla con la versión del firmware

// -----------------------------------------------------------------------------
// Botón PRG (GPIO0, activo en bajo)
// GPIO0 es pin de strapping: si se mantiene PRG al resetear, el ESP32 entra en
// modo descarga. En funcionamiento normal se puede leer sin problemas.
// -----------------------------------------------------------------------------
constexpr int BUTTON_PRG_PIN = 0;
constexpr uint32_t BUTTON_POLL_MS = 10;
constexpr uint32_t BUTTON_DEBOUNCE_MS = 30;
// Una pulsación más larga que esto (y más corta que la larga) se descarta:
// permite arrepentirse de un cambio de modo soltando antes de tiempo.
constexpr uint32_t BUTTON_CLICK_MAX_MS = 800;
constexpr uint32_t BUTTON_DOUBLE_PRESS_GAP_MS = 400;  // Espera de la 2ª pulsación
constexpr uint32_t BUTTON_LONG_PRESS_MS = 5000;       // Mantener para cambiar de modo

// -----------------------------------------------------------------------------
// Tarea GPS (FreeRTOS) y tiempos AT (ms)
// -----------------------------------------------------------------------------
constexpr uint32_t GPS_TASK_STACK_BYTES = 6144;
constexpr UBaseType_t GPS_TASK_PRIORITY = 1;
constexpr BaseType_t GPS_TASK_CORE = 0;  // loop() (LoRa, OLED) corre en el core 1

constexpr uint32_t AT_DEFAULT_TIMEOUT_MS = 1000;
constexpr uint32_t AT_GNSS_TIMEOUT_MS = 2000;
constexpr uint8_t AT_SYNC_ATTEMPTS = 5;  // "AT" repetidos para sincronizar autobaud

constexpr uint32_t GPS_POLL_INTERVAL_MS = 2000;     // Consulta AT+CGNSINF
constexpr uint32_t ERROR_RETRY_INTERVAL_MS = 5000;  // Reintento tras un error
constexpr uint8_t MAX_CONSECUTIVE_AT_FAILURES = 3;  // Antes de declarar error
