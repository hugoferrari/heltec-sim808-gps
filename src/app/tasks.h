#pragma once

#include <Arduino.h>

#include "app/modes.h"

// Tareas cooperativas (TaskScheduler) que corren en loop(), core 1:
//
//   tDecrementarEspera  1 s     Cuenta regresiva y reintento del PDR
//   tEsperarAck         500 ms  Espera el ACK del uplink confirmado del PDR
//   tEnvio              10/30/60 s  PaqueteSalida() en modo "Envío automático"
//
// El PDR (uplink confirmado con ACK) debe aprobarse antes de enviar mediciones,
// tanto automáticas como disparadas con PRG. En modo "Envío apagado" no se
// transmite nada: el PDR pendiente se pausa y se retoma al salir de ese modo.
//
// El GPS (gps/gps_task.h) y el botón (hal/button.h) tienen sus propias tareas FreeRTOS.

void tasksInit();
void tasksExecute();

// Ajusta PDR y envío periódico al modo indicado.
void tasksAplicarModo(Modo modo, uint32_t intervaloMs);

// Envía un uplink de medición ahora. false si el PDR todavía no tiene ACK.
bool tasksDispararUplink();

bool tasksPdrOk();

// Uplink de medición "tx,rx,seq,sf,snr,lat,long".
void PaqueteSalida();
