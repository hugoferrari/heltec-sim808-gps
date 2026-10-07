#pragma once

#include "hal/button.h"

// Interfaz de usuario: decide qué pantalla mostrar en la OLED.
//
//   - Pantalla principal (medición + modo + GPS), refrescada cada DISPLAY_REFRESH_MS.
//   - Avisos temporales (UI_OVERLAY_MS): modo nuevo, intervalo, envío disparado.
//   - Progreso mientras se mantiene PRG para cambiar de modo.
//
// Llamar uiUpdate() en cada loop().

void uiInit(const Button &button);
void uiUpdate();

// Redibuja la pantalla principal ya (si no hay un aviso en curso).
void uiRefreshNow();

// Avisos temporales. Se dibujan de inmediato.
void uiShowMode();      // Modo actual y qué hace el botón en ese modo
void uiShowInterval();  // Intervalo actual del envío automático
void uiShowMessage(const char *title, const char *line1, const char *line2);
