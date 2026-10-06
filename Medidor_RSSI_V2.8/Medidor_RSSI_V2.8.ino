//Segunda version envia y recibe paquetes
//Uplink: "tx,rx,seq,sf,snr"  ejemplo "-115,-98,15,7,-3.25"
//Downlink de Node-RED: "tx,gateway,seq"  ejemplo "-115,macrointell,15"
//Al llegar el downlink se leen RX (getRssi) y SNR (getPktSnrRaw).
//El SF del par es el que tenia el nodo al enviar el uplink que produjo ese TX.
//Medidor RSSI con heltec lora esp32 
//Creado por German Mizdraji para Macro Intell S.A
//
//Version 2.8 heltek - 2026
//Reestructuración, se crean funciones de configuración para un código más limpio.

#include <TaskScheduler.h>
#include <lorawan.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "config.h"
#include "Vars.h"
#include "Display.h"
#include "Task.h"

void setup() {
  Serial.begin(SERIAL_SPEED);
  
  config_OLED();
  init_OLED();

  attachInterrupt(digitalPinToInterrupt(RFM_pins.DIO0), onReceive,  RISING);            //habilita interrupciones para mensajes recibidos lora, se utiliza CHANGE para cuando la señal cambia HIGH <-->LOW. Con RISING se generan multiples interrupciones.

  //Setup LoRa
  config_lora();

  config_task();          // config Scheduler
  
}

void loop() 
{
    PDR.execute(); // Es necesario ejecutar el runner en cada loop
    lora.update();

  recvStatus = 0;

     if(packetReceived) {
        int n = lora.readData(datoEntrante);
        if (n > 0) {
          packetReceived = false;
          // Solo un downlink "-115,gateway" actualiza TX, RX y el display.
          if (ProcesarDatoEntrante(datoEntrante, n, &rssi_rcv, &rssiValue, &get_name)) {
            DEBUG_PRINT("Datoentrante ok, rssi_rcv: "); DEBUG_PRINTLN(rssi_rcv);
            DEBUG_PRINT("rssiValue: "); DEBUG_PRINTLN(rssiValue);
            DEBUG_PRINT("get_name: "); DEBUG_PRINTLN(get_name);
            mostrarDisplay(rssi_rcv, rssiValue, &get_name, seq_medicion, sf_medicion, snr_raw);
          }
        }
     }
     
}




  
