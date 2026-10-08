// Medidor RSSI LoRaWAN + GPS SIM808 - Heltec WiFi LoRa 32 V2
//
// Mide el enlace LoRaWAN (RSSI/SNR de ida y vuelta con Node-RED) y agrega al
// uplink la posición GNSS leída del SIM808. Muestra todo en la OLED integrada.
// El botón PRG elige el modo: disparo manual, envío automático o apagado.
//
// Basado en Medidor RSSI V2.8 (German Mizdraji, Macro Intell S.A.).
//
// Organización:
//   core 1 - loop():       radio LoRaWAN, TaskScheduler (PDR, envíos), modos, OLED
//   core 1 - tarea button: lectura del botón PRG (eventos por cola)
//   core 0 - tarea GPS:    SIM808 por UART2 (comandos AT bloqueantes)

#include <Arduino.h>

#include "app/modes.h"
#include "app/tasks.h"
#include "config.h"
#include "display/display.h"
#include "gps/gps_task.h"
#include "hal/button.h"
#include "log.h"
#include "lora/lora_node.h"
#include "ui/ui.h"

namespace {

Button prgButton(BUTTON_PRG_PIN, true,
                 ButtonTiming{BUTTON_POLL_MS, BUTTON_DEBOUNCE_MS, BUTTON_CLICK_MAX_MS,
                              BUTTON_DOUBLE_PRESS_GAP_MS, BUTTON_LONG_PRESS_MS});

}  // namespace

void setup() {
    Serial.begin(DEBUG_SERIAL_BAUD);
    LOG("APP", "%s", FW_VERSION);

    if (!displayInit()) {
        LOG("APP", "Continuing without OLED");
    }
    displayLogo();

    gpsTaskStart();  // El GPS empieza a buscar fix mientras se muestran las pantallas de inicio
    delay(SPLASH_LOGO_MS);
    displayStartup();
    delay(SPLASH_DURATION_MS);

    if (!loraInit()) {
        LOG("APP", "Continuing without LoRa radio");
    }
    tasksInit();
    modesInit();
    prgButton.begin();
    uiInit(prgButton);
}

void loop() {
    tasksExecute();
    if (loraProcess()) {
        uiRefreshNow();
    }

    ButtonEvent event;
    while (prgButton.getEvent(event)) {
        modesHandleEvent(event);
    }
    uiUpdate();
}
