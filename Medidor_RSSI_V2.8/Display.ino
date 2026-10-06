//Función Setup OLED
void config_OLED()
{
  pinMode(OLED_RST,OUTPUT);
  digitalWrite(OLED_RST, LOW);              // set GPIO16 low to reset OLED
  delay(20); 
  digitalWrite(OLED_RST, HIGH);
  
  //initialize OLED
  Wire.begin(OLED_SDA, OLED_SCL);
  if(!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR, false, false)) {       // Address 0x3C for 128x32
     DEBUG_PRINTLN(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  display.display();
  delay(2000); // Pause for 2 seconds
}

//Inicialización OLED
void init_OLED()
{
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);
  display.print(VERSION);
  display.display();
  display.setTextSize(1);
  display.setCursor(0,40);
  display.print("Desarrollado por Macro Intell.");
  display.display();
  delay(1000);
  display.clearDisplay();
}

void mostrarDisplay(int16_t rssiTX, int16_t rssiRX, String *gatewayname, uint32_t seq, uint8_t sf, int8_t snrRaw){
    char snrTxt[8];
    formatoSnr(snrRaw, snrTxt, sizeof(snrTxt));

    DEBUG_PRINT("Tx: ");DEBUG_PRINTLN(rssiTX);
    display.setCursor(0,0);
    display.setTextSize(2);
    display.print("Tx: ");display.print(rssiTX);

    display.setTextSize(2);
    display.setCursor(0,20);
    DEBUG_PRINT("Rx: ");DEBUG_PRINTLN(rssiRX);
    display.print("Rx: ");display.print(rssiRX);

    display.setTextSize(1);
    display.setCursor(0,42);
    display.print("SF");
    display.print(sf);
    display.print(" SNR ");
    display.print(snrTxt);
    display.print(" #");
    display.print(seq);

    display.setCursor(0,54);
    display.print(*gatewayname);
    DEBUG_PRINT("gatewayname: ");DEBUG_PRINTLN(*gatewayname);
    DEBUG_PRINT("seq: ");DEBUG_PRINTLN(seq);
    DEBUG_PRINT("sf: ");DEBUG_PRINTLN(sf);
    DEBUG_PRINT("snr: ");DEBUG_PRINTLN(snrTxt);
    display.display();
    display.clearDisplay();
}

void ProcesarFalla(){
  DEBUG_PRINT("...........................................");
  display.setTextSize(2);
  display.setCursor(0,35);
  display.print("Rx: .....");
  display.display();
  display.clearDisplay();
}
