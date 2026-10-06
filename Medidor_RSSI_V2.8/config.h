#define VERSION "Medidor RSSI V2.8.1 Heltek"       //Version
#define SERIAL_SPEED 115200         //Serial

#define DEBUG 1   // cambiar a 0 para desactivar
#if DEBUG
  #define DEBUG_PRINT(x)   Serial.print(x)
  #define DEBUG_PRINTLN(x) Serial.println(x)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
#endif

//ABP Credentials 
const char *devAddr = "01fa9919";       //"ef491cef": 24337; "01fa9919": nodo Medidor rssi
const char *nwkSKey = "D8E5A8E01BDDFAF48E059992119BE745";
const char *appSKey = "3F19E1EF0DA5286E178C5B84FC9E23AE";
const char *DeviceNM = "Nodo Medidor RSSI";

#define tipo_modulo 1     //Es para definir el pinout de SPI con el modulo Lora. 0: ESP12, 1: ESP32

#if tipo_modulo == 0
//RFM95W + ESP12 (Macro version)
const sRFM_pins RFM_pins = {
  .CS = 15,
  .RST = 0,
  .DIO0 = 5,
  .DIO1 = 5,
  .DIO2 = 5,
  .DIO5 = -1,
};
#endif

#if tipo_modulo == 1
//SX1276 + ESP32 (TTGO Lora32)
//heltec wifi lora esp32 V2
const sRFM_pins RFM_pins = {
  .CS = 18,
  .RST = 14,
  .DIO0 = 26,
  .DIO1 = 33,
  .DIO2 = 32,
  .DIO5 = -1,
};
#endif

void config_lora();
