void config_task() {
  DEBUG_PRINTLN("Initialized scheduler");

  //PDR.addTask(tIntentarEnvio);
  PDR.addTask(tDecrementarEspera);
  PDR.addTask(tEsperarAck);
  PDR.addTask(tEnvio);
}

// ==================== FUNCIONES DE PDR ==================== //
void decrementarEspera() {
  if (!nodo.pdr_ok) {
    if (nodo.t_wait > 0) {
      nodo.t_wait--;
    }
    else {
      tDecrementarEspera.disable();       //---> ejecuta la funcion intentarEnvio, cuando tDecrementarEspera es deshabilitada.
    }
  }
}

void intentarEnvioPDR() {
  if (!nodo.pdr_ok && nodo.t_wait == 0) {
    nodo.t_wait = random(MIN_RANDOM, MAX_RANDOM);
    armarPayloadMedicion();
    DEBUG_PRINT("Uplink medicion: "); DEBUG_PRINTLN(rssiSend);
    lora.sendUplink(rssiSend, strlen(rssiSend), 1, 1);
    tEsperarAck.enable();
  }
}

void EsperarAck() {
  static uint8_t cont_timeout = 0;
  if (lora.readAck()) {
    DEBUG_PRINTLN("--> ACK recibido");
    nodo.pdr_ok = true;
    tEsperarAck.disable();
    tEnvio.enable();
  } else {
    cont_timeout++;
    if (cont_timeout >= timeout) {
      DEBUG_PRINTLN("--> NO ACK, time out");
      nodo.pdr_ok = false;
      cont_timeout = 0;
      tEsperarAck.disable();
      nodo.t_wait = random(MIN_RANDOM,MAX_RANDOM);
      tDecrementarEspera.enable();
    }
  }
}

void PaqueteSalida() {
  static uint8_t send_count = 0;            //variable estatica para contar las veces que se envia el paquete.
  armarPayloadMedicion();                   //mismo formato "tx,rx" que el sondeo de red
  DEBUG_PRINT("Uplink medicion: "); DEBUG_PRINTLN(rssiSend);
  lora.sendUplink(rssiSend, strlen(rssiSend), 0, 1);
  send_count++;
  DEBUG_PRINT("send_count: "); DEBUG_PRINTLN(send_count);
  DEBUG_PRINT("rcv_count: "); DEBUG_PRINTLN(rcv_count);
  if ((rcv_count - send_count) > 7) {        //si se recibio 8 paquetes de mas, reseteo contadores para igualarlos, asi cuando haya una falla no demora en que send_count supere a rcv_count.
    rcv_count = 0;
    send_count = 0;
  }
  if((send_count - rcv_count) > diferencia){     //solo si se envia mas de lo que se recibe (>3) proceso la falla y reseteo contadores. 
    rcv_count = 0;
    send_count = 0;
    ProcesarFalla();
  }
}