const unsigned long interval = 15000;      // 15 s interval to send message
unsigned long previousMillis = 0;          // will store last time message sent
unsigned int counter = 0;                  // message counter
uint8_t rcv_count  = 0;                        // contador de mensajes recibidos
//uint8_t send_count = 0;                        // contador de mensajes enviados
const uint8_t diferencia = 3;                        //diferencia para comparar send_count y rcv_count

int16_t rssi_rcv  = 0;                     //TX: RSSI con el que el gateway recibio el uplink anterior.
int16_t rssiValue = 0;                     //RX: RSSI con el que el nodo recibio el downlink de esa medicion.
int8_t snr_raw = 0;                        //SNR de ese downlink, en pasos de 0.25 dB (SNR dB = snr_raw / 4).
uint32_t seq_medicion = 0;                 //secuencia que vino en el downlink y se devuelve en el uplink.
uint8_t sf_medicion = 0;                   //SF del uplink al que pertenece el TX.
uint8_t sf_en_aire = 0;                    //SF configurado en el ultimo uplink, todavia sin downlink.
char rssiSend[40] = {0};                   //uplink "tx,rx,seq,sf,snr", ej: "-115,-98,15,7,-3.25"
String get_name;                           //variable para guardar gatewayname

#define INPUTBUFF 100
char datoEntrante[INPUTBUFF]={0};          //variable para guardar dato recibido
              

byte recvStatus = 0;
bool packetReceived = false;

// variables para timeout
#define tick_time 100  // base de tiempo para el delay
#define timeout 150    // timeout * tick_time = tiempo de time out = 8 segundos



bool resultado_envio = false;

// INTERVALOS (en segundos)
#define LONG_TIME_TO_WAIT 600    // son 10 minutos, en s
#define UN_DIA            86400  // 24hs en s

#define MIN_RANDOM        5     //s, 10 segundos
#define MAX_RANDOM        10     //s, 60 segundos
#define MIN_RANDOM_LARGO  120    //s, 2 minutos
#define MAX_RANDOM_LARGO  600    //s, 10 minutos

//REINTENTOS
#define MAX_REINTENTOS 6    //es la cantidad  maxima de reintentos que hace con un intervalo de tiempo pequeño, luego espera un tiempo mas largo y vuelve a intentar. Min 6 para que use todos los SF

#define MAX_PAUSAS_LARGAS 3 //Maximo de pausas largas que se realizan cuando los reintentos cortos fallan, superado este numero se espera un dia completo

// estructura para guardar el estado del nodo
struct str {
  bool pdr_ok = false;                  // =1 cuando la prueba de red dio ok, sino =0
  
  int32_t t_wait = 0;                   // tiempo de espera
  bool pausa_larga = false;              //=1 esperar tiempo largo
  
  uint8_t cont_reintento_corto = 0;     // usado para contar los intentos uplink de las funciones pdr, sync e ident
  uint8_t cont_pausas_largas = 0;       // cuenta las pausas largas que se realizan cuando los reintentos cortos fallan
};  str nodo;

void IRAM_ATTR onReceive();
bool ProcesarDatoEntrante(char *inputData, int len, int16_t *rssi_out, int16_t *rssiRX, String *name_out);
void armarPayloadMedicion();
void formatoSnr(int8_t raw, char *out, size_t n);