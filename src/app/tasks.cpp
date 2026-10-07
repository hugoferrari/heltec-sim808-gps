#include "tasks.h"

// TaskScheduler.h contiene la implementación: incluirlo en un solo .cpp.
#include <TaskScheduler.h>
#include <lorawan.h>

#include "config.h"
#include "log.h"
#include "lora/medicion.h"
#include "ui/ui.h"

namespace {

// Estado de la prueba de red (PDR): un uplink confirmado que debe recibir ACK
// antes de empezar a enviar mediciones.
struct EstadoNodo {
    bool pdr_ok = false;  // true cuando el PDR recibió ACK
    int32_t t_wait = 0;   // segundos hasta el próximo intento de PDR
} nodo;

Modo modo = Modo::Apagado;   // Hasta que modesInit() aplique el modo inicial
uint8_t send_count = 0;      // Uplinks de medición enviados (detección de falla)

void decrementarEspera();
void intentarEnvioPDR();
void EsperarAck();

Scheduler runner;

// Task(intervalo ms, iteraciones, callback, scheduler, habilitada, onEnable, onDisable)
// El PDR arranca cuando tasksAplicarModo() habilita tDecrementarEspera.
Task tDecrementarEspera(1000, TASK_FOREVER, &decrementarEspera, &runner, false, nullptr, &intentarEnvioPDR);
Task tEsperarAck(ACK_POLL_INTERVAL_MS, TASK_FOREVER, &EsperarAck, &runner, false);
Task tEnvio(ENVIO_INTERVALOS_MS[0], TASK_FOREVER, &PaqueteSalida, &runner, false);

bool transmisionHabilitada() {
    return modo != Modo::Apagado;
}

// ==================== PDR ==================== //

void decrementarEspera() {
    if (nodo.pdr_ok) {
        return;
    }
    if (nodo.t_wait > 0) {
        nodo.t_wait--;
    } else {
        tDecrementarEspera.disable();  // onDisable -> intentarEnvioPDR()
    }
}

void intentarEnvioPDR() {
    // onDisable también se ejecuta al pausar el PDR en modo apagado.
    if (transmisionHabilitada() && !nodo.pdr_ok && nodo.t_wait == 0) {
        nodo.t_wait = random(PDR_RETRY_MIN_S, PDR_RETRY_MAX_S);
        armarPayloadMedicion();
        LOG("LORA", "Uplink PDR (confirmado): %s", rssiSend);
        lora.sendUplink(rssiSend, strlen(rssiSend), 1, LORA_PORT);
        tEsperarAck.enable();
    }
}

void EsperarAck() {
    static uint16_t cont_timeout = 0;
    if (lora.readAck()) {
        LOG("LORA", "--> ACK recibido");
        nodo.pdr_ok = true;
        cont_timeout = 0;
        tEsperarAck.disable();
        if (modo == Modo::Automatico) {
            tEnvio.enable();
        }
        uiRefreshNow();
        return;
    }
    if (++cont_timeout >= ACK_TIMEOUT_TICKS) {
        LOG("LORA", "--> NO ACK, time out");
        nodo.pdr_ok = false;
        cont_timeout = 0;
        tEsperarAck.disable();
        nodo.t_wait = random(PDR_RETRY_MIN_S, PDR_RETRY_MAX_S);
        tDecrementarEspera.enable();
    }
}

// Arranca el PDR si no está aprobado ni en curso.
void retomarPdr() {
    if (nodo.pdr_ok || tDecrementarEspera.isEnabled() || tEsperarAck.isEnabled()) {
        return;
    }
    nodo.t_wait = 0;  // Primer intento inmediato
    tDecrementarEspera.enable();
}

void pausarPdr() {
    tDecrementarEspera.disable();  // intentarEnvioPDR() no envía en modo apagado
    tEsperarAck.disable();
}

// ==================== Envío de mediciones ==================== //

// Demasiados envíos sin downlink: se marca la falla en pantalla ("Rx: ....").
void ProcesarFalla() {
    LOG("LORA", "Falla: %u envios sin respuesta", MAX_DIFERENCIA_ENVIOS + 1);
    medicion.falla = true;
    uiRefreshNow();
}

// Si tEnvio ya corre, setInterval() reprograma el próximo envío con el nuevo
// intervalo. Si no, enable() hace el primer envío de inmediato.
void configurarEnvioAutomatico(uint32_t intervaloMs) {
    tEnvio.setInterval(intervaloMs);
    if (!tEnvio.isEnabled() && nodo.pdr_ok) {
        tEnvio.enable();
    }
}

}  // namespace

void PaqueteSalida() {
    armarPayloadMedicion();  // "tx,rx,seq,sf,snr,lat,long"
    LOG("LORA", "Uplink medicion: %s", rssiSend);
    lora.sendUplink(rssiSend, strlen(rssiSend), 0, LORA_PORT);
    send_count++;
    LOG("LORA", "send_count: %u  rcv_count: %u", send_count, rcv_count);

    // Si se recibieron 8 paquetes de más, se igualan los contadores para que una
    // falla futura se detecte sin demora.
    if ((rcv_count - send_count) > 7) {
        rcv_count = 0;
        send_count = 0;
    }
    // Solo si se envía más de lo que se recibe se procesa la falla.
    if ((send_count - rcv_count) > MAX_DIFERENCIA_ENVIOS) {
        rcv_count = 0;
        send_count = 0;
        ProcesarFalla();
    }
}

void tasksAplicarModo(Modo nuevo, uint32_t intervaloMs) {
    const bool cambioDeModo = nuevo != modo;
    modo = nuevo;
    if (cambioDeModo) {
        // Cada modo empieza una detección de falla nueva.
        send_count = 0;
        rcv_count = 0;
        medicion.falla = false;
    }

    switch (modo) {
        case Modo::Apagado:
            pausarPdr();
            tEnvio.disable();
            break;
        case Modo::DisparoPrg:
            retomarPdr();
            tEnvio.disable();
            break;
        case Modo::Automatico:
            retomarPdr();
            configurarEnvioAutomatico(intervaloMs);
            break;
    }
}

bool tasksDispararUplink() {
    if (!transmisionHabilitada() || !nodo.pdr_ok) {
        return false;
    }
    PaqueteSalida();
    return true;
}

bool tasksPdrOk() {
    return nodo.pdr_ok;
}

void tasksInit() {
    LOG("APP", "Initialized scheduler");
    runner.startNow();
}

void tasksExecute() {
    runner.execute();
}
