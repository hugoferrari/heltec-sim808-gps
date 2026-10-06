//Configuración lora
void config_lora()
{
  Serial.println("Start..");
  if(!lora.init()){
    DEBUG_PRINTLN("RFM95 not detected");
    delay(5000);
    return;
  }
  lora.setDeviceClass(CLASS_C);     // Set LoRaWAN Class change CLASS_A or CLASS_C
  lora.setDataRate(SF7BW125);       // SF de escucha. El SF de cada uplink lo rota armarPayloadMedicion.
  lora.setChannel(CH0);            //Set Channel Default
  lora.setNwkSKey(nwkSKey);
  lora.setAppSKey(appSKey);
  lora.setDevAddr(devAddr);
  DEBUG_PRINT("Device name: "); DEBUG_PRINTLN(DeviceNM);
  DEBUG_PRINT("nwkSKey: "); DEBUG_PRINTLN(nwkSKey);
  DEBUG_PRINT("appSKey: "); DEBUG_PRINTLN(appSKey);
  DEBUG_PRINT("devAddr: "); DEBUG_PRINTLN(devAddr);
}