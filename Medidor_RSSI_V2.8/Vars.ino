//FUNCIONES AUXILIARES

// true solo si el payload es "rssi,gateway,contador", por ejemplo "-115,macrointell,15".
// Sin el contador numerico final se descarta. Un comando MAC (0x03) tampoco cumple esto.
bool esMedicionValida(const char *data) {
  if (data == NULL || data[0] != '-') return false;
  const char *p = data + 1;
  if (*p < '0' || *p > '9') return false;
  while (*p >= '0' && *p <= '9') p++;
  if (*p != ',') return false;
  p++;
  while (*p == ' ') p++;
  if (*p == '\0' || *p == ',') return false;
  while (*p != '\0' && *p != ',') p++;
  if (*p != ',') return false;
  p++;
  while (*p == ' ') p++;
  if (*p < '0' || *p > '9') return false;
  while (*p >= '0' && *p <= '9') p++;
  return *p == '\0';
}

void debugDownlinkIgnorado(const char *data, int len) {
#if DEBUG
  Serial.print("Downlink ignorado, hex:");
  if (len < 0) len = 0;
  if (len > 16) len = 16;
  for (int i = 0; i < len; i++) {
    uint8_t b = (uint8_t)data[i];
    Serial.print(b < 16 ? " 0" : " ");
    Serial.print(b, HEX);
  }
  Serial.println();
#else
  (void)data;
  (void)len;
#endif
}

bool ProcesarDatoEntrante(char *inputData, int len, int16_t *rssi_out, int16_t *rssiRX, String *name_out){
  //inputData: datoEntrada, dato recibido completo.
  //rssi_out: rssi recibido (es el valor que llego al gateway, seria el TX del nodo).
  //rssiRX: es el valor rssi leido en el nodo cuando llega un paquete.
  //name_out: es el nombre del gateway.
  //formato del datoEntrante: rssi,gateway,seq  ej: -115,macrointell,15
  char *token;

  if (!esMedicionValida(inputData)) {
    debugDownlinkIgnorado(inputData, len);
    memset(inputData, 0, INPUTBUFF);
    return false;
  }

  // Obtener el primer token (rssi)
  token = strtok(inputData, ",");
  if (token != NULL) {
    int rssi = atoi(token);
    *rssi_out = rssi;             //valor rssi, salida por puntero
  }

  // Obtener el segundo token (gatewayname)
  token = strtok(NULL, ",");
  if (token != NULL) {
    *name_out = String(token);    //nombre gateway, salida por puntero
    rcv_count++;    //solo aumento cuando el datorecibido posee una coma
  }

  // Tercer token opcional: secuencia que Node-RED quiere ver de vuelta.
  token = strtok(NULL, ",");
  if (token != NULL) {
    while (*token == ' ') token++;
    seq_medicion = (uint32_t)strtoul(token, NULL, 10);
  } else {
    seq_medicion = 0;
  }

  *rssiRX = lora.getRssi();
  snr_raw = lora.getPktSnrRaw();
  // El TX de este downlink es el RSSI del uplink anterior, enviado con sf_en_aire.
  sf_medicion = sf_en_aire;

  memset(inputData, 0, INPUTBUFF);           //Se limpia el buffer del datoEntrante.
  return true;
}

// SNR dB = raw / 4. Se escribe con dos decimales, por ejemplo -3.25.
void formatoSnr(int8_t raw, char *out, size_t n) {
  int cents = (int)raw * 25;
  int neg = cents < 0;
  int a = abs(cents);
  snprintf(out, n, "%s%d.%02d", neg ? "-" : "", a / 100, a % 100);
}

uint8_t sfDeDataRate(unsigned char dr) {
  switch (dr) {
    case SF10BW125: return 10;
    case SF9BW125:  return 9;
    case SF8BW125:  return 8;
    case SF7BW125:  return 7;
    default:        return 0;
  }
}

// AU915, canales de 125 kHz: SF7, SF8, SF9, SF10 y vuelve a SF7.
// El data rate de escucha no se toca, para seguir recibiendo el downlink.
void prepararSfDeEnvio() {
  static const uint8_t ciclo[] = { SF7BW125, SF8BW125, SF9BW125, SF10BW125 };
  static uint8_t idx = 0;
  lora.setTxDataRate(ciclo[idx]);
  idx = (idx + 1) % (sizeof(ciclo) / sizeof(ciclo[0]));
  sf_en_aire = sfDeDataRate(lora.getDataRate());
  DEBUG_PRINT("SF envio: ");
  DEBUG_PRINTLN(sf_en_aire);
}

// Uplink: "tx,rx,seq,sf,snr". Ejemplo: "-115,-98,15,7,-3.25"
// seq, TX, RX y SNR son del downlink anterior.
// sf es el spreading factor con el que salio el uplink que produjo ese TX.
// Despues de armar el texto se elige el SF del envio que sale ahora.
void armarPayloadMedicion() {
  char snrTxt[8];
  formatoSnr(snr_raw, snrTxt, sizeof(snrTxt));
  snprintf(rssiSend, sizeof(rssiSend), "%d,%d,%lu,%u,%s",
           (int)rssi_rcv, (int)rssiValue, (unsigned long)seq_medicion,
           (unsigned)sf_medicion, snrTxt);
  prepararSfDeEnvio();
}

// Función de interrupción para mensajes recibidos lora
void IRAM_ATTR onReceive() 
{
  if(!packetReceived){
   packetReceived = true;
  }
}
    