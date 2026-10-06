#include "lora_node.h"

#include "config.h"
#include "log.h"
#include "medicion.h"
#include "secrets.h"

// Pines del SX1276 que usa la librería Beelan (declarado extern en lorawan.h).
const sRFM_pins RFM_pins = {
    .CS = LORA_CS_PIN,
    .RST = LORA_RST_PIN,
    .DIO0 = LORA_DIO0_PIN,
    .DIO1 = LORA_DIO1_PIN,
    .DIO2 = LORA_DIO2_PIN,
    .DIO5 = -1,
};

namespace {

volatile bool packetReceived = false;
char datoEntrante[INPUT_BUFF_SIZE] = {0};

// DIO0 sube al terminar una recepción (RxDone).
void IRAM_ATTR onReceive() {
    packetReceived = true;
}

}  // namespace

bool loraInit() {
    if (!lora.init()) {
        LOG("LORA", "ERROR: SX1276/RFM95 not detected");
        return false;
    }
    lora.setDeviceClass(CLASS_C);  // Escucha continua de downlinks
    lora.setDataRate(SF7BW125);    // SF de escucha; el SF de cada uplink lo rota armarPayloadMedicion()
    lora.setChannel(CH0);
    lora.setNwkSKey(LORAWAN_NWKSKEY);
    lora.setAppSKey(LORAWAN_APPSKEY);
    lora.setDevAddr(LORAWAN_DEVADDR);

    attachInterrupt(digitalPinToInterrupt(RFM_pins.DIO0), onReceive, RISING);

    LOG("LORA", "Device: %s  DevAddr: %s", LORAWAN_DEVICE_NAME, LORAWAN_DEVADDR);
    return true;
}

bool loraProcess() {
    lora.update();

    if (!packetReceived) {
        return false;
    }
    const int n = lora.readData(datoEntrante);
    if (n <= 0) {
        return false;  // update() todavía no entregó el paquete; se reintenta en el próximo loop
    }
    packetReceived = false;
    // Solo un downlink "tx,gateway,seq" actualiza la medición y el display.
    return ProcesarDatoEntrante(datoEntrante, n);
}
