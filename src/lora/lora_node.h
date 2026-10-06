#pragma once

#include <lorawan.h>

// Radio LoRaWAN (Beelan, ABP, AU915) sobre el SX1276 integrado del Heltec V2.

// Inicializa la radio y la sesión ABP (antes config_lora()).
// Devuelve false si no se detecta el SX1276.
bool loraInit();

// Llamar en cada loop(): ejecuta lora.update() y procesa downlinks.
// Devuelve true si llegó un downlink de medición válido.
bool loraProcess();
