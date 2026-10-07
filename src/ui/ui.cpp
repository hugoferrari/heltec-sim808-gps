#include "ui.h"

#include "app/modes.h"
#include "app/tasks.h"
#include "config.h"
#include "display/display.h"
#include "gps/gps_task.h"
#include "lora/medicion.h"

namespace {

const Button *prgButton = nullptr;

uint32_t lastDrawMs = 0;
uint32_t overlayUntilMs = 0;
bool overlayActive = false;
bool holdShown = false;     // Se está mostrando el progreso de la pulsación larga
bool overlayFresh = false;  // Aviso dibujado después del último uiUpdate()

void drawMain() {
    char tag[16];
    switch (modoActual()) {
        case Modo::DisparoPrg: snprintf(tag, sizeof(tag), "PRG"); break;
        case Modo::Automatico:
            snprintf(tag, sizeof(tag), "AUTO %lus", static_cast<unsigned long>(intervaloActualMs() / 1000));
            break;
        case Modo::Apagado: snprintf(tag, sizeof(tag), "OFF"); break;
    }
    displayMain(medicion, gpsGetStatus(), tag, tasksPdrOk());
    lastDrawMs = millis();
}

void startOverlay() {
    overlayActive = true;
    overlayFresh = true;
    overlayUntilMs = millis() + UI_OVERLAY_MS;
    lastDrawMs = millis();
}

void holdHint(char *buf, size_t n) {
    snprintf(buf, n, "Mantener PRG %lus: modo", static_cast<unsigned long>(BUTTON_LONG_PRESS_MS / 1000));
}

}  // namespace

void uiInit(const Button &button) {
    prgButton = &button;
    drawMain();
}

void uiUpdate() {
    const uint32_t now = millis();
    const bool fresh = overlayFresh;
    overlayFresh = false;

    const uint32_t held = prgButton ? prgButton->holdMs() : 0;
    if (held > BUTTON_CLICK_MAX_MS) {
        if (!holdShown || now - lastDrawMs >= DISPLAY_FAST_REFRESH_MS) {
            displayHoldProgress(static_cast<float>(held) / BUTTON_LONG_PRESS_MS,
                                modoNombre(modoSiguiente(modoActual())));
            holdShown = true;
            lastDrawMs = now;
        }
        return;
    }

    if (holdShown) {
        holdShown = false;
        // Tras un LongPress, modes ya dibujó el aviso del modo nuevo (recién iniciado).
        // Si se soltó antes de tiempo, se vuelve a la pantalla principal.
        if (!fresh) {
            overlayActive = false;
            drawMain();
        }
        return;
    }

    if (overlayActive) {
        if (static_cast<int32_t>(now - overlayUntilMs) >= 0) {
            overlayActive = false;
            drawMain();
        }
        return;
    }

    if (now - lastDrawMs >= DISPLAY_REFRESH_MS) {
        drawMain();
    }
}

void uiRefreshNow() {
    if (!overlayActive && !holdShown) {
        drawMain();
    }
}

void uiShowMode() {
    char hint[40];
    holdHint(hint, sizeof(hint));
    const Modo modo = modoActual();
    switch (modo) {
        case Modo::DisparoPrg:
            displayInfo(modoNombreCorto(modo), "1 clic: enviar uplink", "", hint);
            break;
        case Modo::Automatico: {
            char intervalo[24];
            snprintf(intervalo, sizeof(intervalo), "Cada %lu s",
                     static_cast<unsigned long>(intervaloActualMs() / 1000));
            displayInfo(modoNombreCorto(modo), intervalo, "2 clics: intervalo", hint);
            break;
        }
        case Modo::Apagado:
            displayInfo(modoNombreCorto(modo), "No se transmite", "Medicion finalizada", hint);
            break;
    }
    startOverlay();
}

void uiShowInterval() {
    char intervalo[24];
    snprintf(intervalo, sizeof(intervalo), "Intervalo: %lu s",
             static_cast<unsigned long>(intervaloActualMs() / 1000));
    displayInfo(modoNombreCorto(Modo::Automatico), intervalo, "", "2 clics: intervalo");
    startOverlay();
}

void uiShowMessage(const char *title, const char *line1, const char *line2) {
    displayInfo(title, line1, line2, "");
    startOverlay();
}
