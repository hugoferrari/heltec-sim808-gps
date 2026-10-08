#include "modes.h"

#include "app/tasks.h"
#include "config.h"
#include "log.h"
#include "ui/ui.h"

namespace {

constexpr size_t NUM_INTERVALOS = sizeof(ENVIO_INTERVALOS_MS) / sizeof(ENVIO_INTERVALOS_MS[0]);

Modo modo = Modo::DisparoPrg;
size_t intervaloIdx = 0;

void aplicar() {
    tasksAplicarModo(modo, ENVIO_INTERVALOS_MS[intervaloIdx]);
}

void cambiarModo() {
    modo = modoSiguiente(modo);
    LOG("MODO", "Modo: %s", modoNombre(modo));
    aplicar();
    uiShowMode();
}

void cambiarIntervalo() {
    intervaloIdx = (intervaloIdx + 1) % NUM_INTERVALOS;
    LOG("MODO", "Intervalo de envio: %lu s",
        static_cast<unsigned long>(ENVIO_INTERVALOS_MS[intervaloIdx] / 1000));
    aplicar();
    uiShowInterval();
}

void dispararUplink() {
    if (tasksDispararUplink()) {
        uiShowMessage("Uplink", "Enviando medicion...", "");
    } else {
        LOG("MODO", "Disparo ignorado: PDR sin ACK todavia");
        uiShowMessage("Sin red", "Esperando ACK del PDR", "Reintente luego");
    }
}

}  // namespace

const char *modoNombre(Modo m) {
    switch (m) {
        case Modo::DisparoPrg: return "Disparar con boton PRG";
        case Modo::Automatico: return "Envio automatico";
        case Modo::Apagado: return "Envio apagado";
    }
    return "?";
}

const char *modoNombreCorto(Modo m) {
    switch (m) {
        case Modo::DisparoPrg: return "Disparo PRG";
        case Modo::Automatico: return "Envio auto";
        case Modo::Apagado: return "Envio apagado";
    }
    return "?";
}

Modo modoSiguiente(Modo m) {
    switch (m) {
        case Modo::DisparoPrg: return Modo::Automatico;
        case Modo::Automatico: return Modo::Apagado;
        case Modo::Apagado: return Modo::DisparoPrg;
    }
    return Modo::DisparoPrg;
}

void modesInit() {
    LOG("MODO", "Modo: %s", modoNombre(modo));
    aplicar();
}

void modesHandleEvent(ButtonEvent event) {
    switch (event) {
        case ButtonEvent::LongPress:
            cambiarModo();
            break;
        case ButtonEvent::DoublePress:
            if (modo == Modo::Automatico) {
                cambiarIntervalo();
            } else {
                uiShowMode();  // Recordar qué hace el botón en este modo
            }
            break;
        case ButtonEvent::SinglePress:
            if (modo == Modo::DisparoPrg) {
                dispararUplink();
            } else {
                uiShowMode();
            }
            break;
    }
}

Modo modoActual() {
    return modo;
}

uint32_t intervaloActualMs() {
    return ENVIO_INTERVALOS_MS[intervaloIdx];
}
