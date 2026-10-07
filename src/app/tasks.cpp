#include "tasks.h"

// TaskScheduler.h contiene la implementación: incluirlo en un solo .cpp.
#include <TaskScheduler.h>
#include <lorawan.h>

#include "config.h"
#include "display/display.h"
#include "log.h"
#include "lora/medicion.h"

namespace {

// Estado de la prueba de red (PDR): un uplink confirmado que debe recibir ACK
// antes de empezar los envíos periódicos.
struct EstadoNodo {
    bool pdr_ok = false;  // true cuando el PDR recibió ACK
    int32_t t_wait = 0;   // segundos hasta el próximo intento de PDR
} nodo;

void decrementarEspera();
void intentarEnvioPDR();
void EsperarAck();
void refrescarDisplay();

Scheduler runner;

// Task(intervalo ms, iteraciones, callback, scheduler, habilitada, onEnable, onDisable)
Task tDecrementarEspera(1000, TASK_FOREVER, &decrementarEspera, &runner, true, nullptr, &intentarEnvioPDR);
Task tEsperarAck(ACK_POLL_INTERVAL_MS, TASK_FOREVER, &EsperarAck, &runner, false);
Task tEnvio(ENVIO_INTERVAL_MS, TASK_FOREVER, &PaqueteSalida, &runner, false);
Task tDisplay(DISPLAY_REFRESH_MS, TASK_FOREVER, &refrescarDisplay, &runner, true);

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
    if (!nodo.pdr_ok && nodo.t_wait == 0) {
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
        tEnvio.enable();
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

// ==================== Envío periódico ==================== //

// Demasiados envíos sin downlink: se marca la falla en pantalla ("Rx: ....").
void ProcesarFalla() {
    LOG("LORA", "Falla: %u envios sin respuesta", MAX_DIFERENCIA_ENVIOS + 1);
    medicion.falla = true;
    displayRefresh();
}

void refrescarDisplay() {
    displayRefresh();
}

}  // namespace

void PaqueteSalida() {
    static uint8_t send_count = 0;  // Uplinks periódicos enviados
    armarPayloadMedicion();         // "tx,rx,seq,sf,snr,lat,long"
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

void tasksInit() {
    LOG("APP", "Initialized scheduler");
    runner.startNow();
}

void tasksExecute() {
    runner.execute();
}
