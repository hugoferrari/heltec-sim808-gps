#pragma once

// Plantilla de credenciales LoRaWAN (ABP).
// Copiar este archivo como include/secrets.h y completar los valores.
// include/secrets.h está en .gitignore: no se sube al repositorio.

#define LORAWAN_DEVICE_NAME "Nodo Medidor RSSI"
#define LORAWAN_DEVADDR "00000000"                          // 4 bytes, hex
#define LORAWAN_NWKSKEY "00000000000000000000000000000000"  // 16 bytes, hex
#define LORAWAN_APPSKEY "00000000000000000000000000000000"  // 16 bytes, hex
