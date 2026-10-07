#pragma once

// Tareas cooperativas (TaskScheduler) que corren en loop(), core 1:
//
//   tDecrementarEspera  1 s     Cuenta regresiva y reintento del PDR
//   tEsperarAck         500 ms  Espera el ACK del uplink confirmado del PDR
//   tEnvio              10 s    PaqueteSalida(): uplink de medición periódico
//   tDisplay            1 s     Refresco de la OLED (RSSI + GPS)
//
// El GPS no está aquí: corre en su propia tarea FreeRTOS (gps/gps_task.h).

void tasksInit();
void tasksExecute();

// Uplink de medición periódico "tx,rx,seq,sf,snr,lat,long".
void PaqueteSalida();
