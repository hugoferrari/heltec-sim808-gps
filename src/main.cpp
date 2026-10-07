// Medidor RSSI LoRaWAN + GPS SIM808 - Heltec WiFi LoRa 32 V2
//
// Mide el enlace LoRaWAN (RSSI/SNR de ida y vuelta con Node-RED) y agrega al
// uplink la posición GNSS leída del SIM808. Muestra todo en la OLED integrada.
//
// Basado en Medidor RSSI V2.8 (German Mizdraji, Macro Intell S.A.).
//
// Organización:
//   core 1 - loop():   radio LoRaWAN + TaskScheduler (PDR, envíos, OLED)
//   core 0 - tarea GPS: SIM808 por UART2 (comandos AT bloqueantes)

#include <Arduino.h>

#include "app/tasks.h"
#include "config.h"
#include "display/display.h"
#include "gps/gps_task.h"
#include "log.h"
#include "lora/lora_node.h"

void setup() {
    Serial.begin(DEBUG_SERIAL_BAUD);
    LOG("APP", "%s", FW_VERSION);

    if (!displayInit()) {
        LOG("APP", "Continuing without OLED");
    }
    displayStartup();

    gpsTaskStart();  // El GPS empieza a buscar fix mientras se muestra el splash
    delay(SPLASH_DURATION_MS);

    if (!loraInit()) {
        LOG("APP", "Continuing without LoRa radio");
    }
    tasksInit();
    displayRefresh();
}

void loop() {
    tasksExecute();
    if (loraProcess()) {
        displayRefresh();
    }
}
