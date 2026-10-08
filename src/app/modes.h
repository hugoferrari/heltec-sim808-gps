#pragma once

#include <Arduino.h>

#include "hal/button.h"

// Modos de operación del Medidor RSSI, controlados con el botón PRG.
//
//   Mantener PRG (BUTTON_LONG_PRESS_MS): cambia de modo
//     Disparo PRG (defecto) -> Envío automático -> Envío apagado -> Disparo PRG
//   1 pulsación en "Disparo PRG": envía un uplink de medición
//   2 pulsaciones en "Envío automático": intervalo 10 s -> 30 s -> 60 s -> 10 s
//
// Al arrancar se usa "Disparo PRG" con el primer intervalo de ENVIO_INTERVALOS_MS.

enum class Modo : uint8_t {
    DisparoPrg,  // Uplink de medición con 1 pulsación de PRG
    Automatico,  // Uplink de medición periódico
    Apagado,     // No transmite (fin de la medición)
};

const char *modoNombre(Modo modo);       // Nombre completo (logs)
const char *modoNombreCorto(Modo modo);  // Título en la OLED
Modo modoSiguiente(Modo modo);

// Aplica el modo por defecto. Llamar después de tasksInit().
void modesInit();

// Procesa un evento del botón PRG según el modo actual.
void modesHandleEvent(ButtonEvent event);

Modo modoActual();
uint32_t intervaloActualMs();
