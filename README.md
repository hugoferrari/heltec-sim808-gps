# Heltec WiFi LoRa 32 V2 + SIM808 GPS + OLED

Firmware para **Heltec WiFi LoRa 32 V2 (ESP32)** que lee la posición del **GNSS integrado del SIM808** mediante comandos AT por UART y la muestra en la **OLED integrada** de la placa.

## Descripción

El ESP32 se comunica con el SIM808 por una UART dedicada (UART2), independiente de la UART0 usada para programación y monitor USB. El firmware:

1. Verifica que el SIM808 responde a `AT`.
2. Enciende el GNSS (`AT+CGNSPWR=1`).
3. Consulta periódicamente la posición (`AT+CGNSINF`, cada 2 s).
4. Determina si hay fix, valida y extrae latitud, longitud, satélites y HDOP.
5. Muestra el estado en la OLED y mensajes de diagnóstico por Serial.

Es robusto frente a la ausencia de fix (el primer fix en frío puede tardar varios minutos), a respuestas inválidas y a la pérdida de comunicación con el SIM808, que se reintenta periódicamente. Solo se usan datos reales del GNSS: no hay coordenadas simuladas ni posicionamiento por red celular.

La arquitectura deja separada la capa AT (`Sim808`) del GNSS (`Sim808Gps`) para poder agregar luego GSM/GPRS (reutilizando la capa AT) y LoRa/LoRaWAN (el radio SX1276 de la placa queda libre).

## Hardware

- **Heltec WiFi LoRa 32 V2** (ESP32 + SX1276 + OLED SSD1306 0.96").
- **Módulo SIM808** (breakout genérico, DFRobot, Adafruit FONA 808 u otro).
- **Antena GPS** conectada al conector *GPS/GNSS* del módulo (no al de GSM). Si es activa, verificar que el módulo la alimente.
- **Fuente para el SIM808**: 3.4–4.4 V capaz de entregar **2 A** de pico (ver [Alimentación](#alimentación)).
- Cables y, si el módulo no tiene adaptación de niveles, un **adaptador de niveles** o divisor resistivo (ver [Niveles lógicos](#niveles-lógicos-uart)).
- Para este firmware no hace falta SIM ni antena GSM (el GNSS funciona sin ellas).

### Diagrama de conexiones

![Diagrama de conexiones Heltec WiFi LoRa 32 V2 + SIM808](DIAGRAMA.png)

En el diagrama, el SIM808 (módulo mini con entrada VCC directa a VBAT) se alimenta con una celda **18650** (3.7–4.2 V) independiente del Heltec. El GND de la batería, el del SIM808 y el del Heltec quedan unidos. Cable amarillo: GPIO23 ← TXD; cable verde: GPIO17 → RXD.

| Heltec WiFi LoRa 32 V2 | SIM808 | Función |
| ---------------------- | ------ | ------- |
| GPIO23 | TXD | UART RX del ESP32 (recibe datos del SIM808) |
| GPIO17 | RXD | UART TX del ESP32 (envía comandos al SIM808) |
| GND | GND | Tierra común (obligatoria) |
| — | VBAT / VIN | Alimentación desde fuente independiente (no desde el Heltec) |
| GPIO22 (opcional, vía NPN) | PWRKEY | Encendido remoto; deshabilitado por defecto |

```
ESP32 RX (GPIO23) ← SIM808 TX (TXD)
ESP32 TX (GPIO17) → SIM808 RX (RXD)
GND Heltec ─────── GND SIM808 ─────── GND fuente SIM808
```

Las líneas se cruzan: el TX de una placa va al RX de la otra. **Ambas placas deben compartir GND**; sin tierra común la UART no funciona.

Estos valores se definen en `include/config.h` (`SIM808_RX_PIN`, `SIM808_TX_PIN`, `SIM808_BAUD`, `SIM808_PWRKEY_PIN`).

#### Por qué GPIO23 y GPIO17

Pines ocupados en el Heltec WiFi LoRa 32 V2 (confirmados con el *variant* `heltec_wifi_lora_32_V2` de Arduino-ESP32 y el pinout de Heltec):

| Uso | GPIO |
| --- | ---- |
| OLED (SDA, SCL, RST) | 4, 15, 16 |
| LoRa SX1276 (SCK, MISO, MOSI, NSS, RST, DIO0, DIO1, DIO2) | 5, 19, 27, 18, 14, 26, 35, 34 |
| Vext (control de 3.3 V externo) | 21 |
| LED blanco | 25 |
| Botón PRG / boot | 0 |
| UART0 USB (TX, RX) | 1, 3 |
| Flash SPI interna | 6–11 |

- **GPIO23** y **GPIO17** están expuestos en el header, no son pines de *strapping* (0, 2, 5, 12, 15), no son solo-entrada (34–39) y no los usa ningún periférico de la placa.
- No se usa el par por defecto de `Serial2` (RX=16, TX=17) porque **GPIO16 es el reset de la OLED**.
- Se evita GPIO13 porque en algunas revisiones de la V2 se usa para medir la batería.
- Si en el futuro se usa I2C externo, no utilizar los pines por defecto de `Wire` del variant (SDA=21, SCL=22): GPIO21 es Vext.

#### PWRKEY (opcional)

Muchos módulos SIM808 se encienden con un botón propio o tienen PWRKEY cableado para arrancar solos; en ese caso no hace falta conectarlo (`SIM808_PWRKEY_PIN = -1`, valor por defecto). Si se quiere encendido remoto:

- PWRKEY se activa llevándolo a GND durante ≥ 1 s. **No conectarlo directo a un GPIO**: usar un transistor NPN (colector a PWRKEY, emisor a GND, base al GPIO22 con ~4.7 kΩ).
- Configurar `SIM808_PWRKEY_PIN = 22`. El firmware pulsa PWRKEY tras dos intentos fallidos de detección.
- El mismo pulso enciende **o apaga** el módulo; si el SIM808 está encendido pero la UART está mal cableada, el pulso lo apagará.

### Alimentación

- El SIM808 trabaja con **VBAT 3.4–4.4 V** (típico 4.0 V, o una celda Li-ion de 3.7 V). Muchos breakouts incluyen un regulador y aceptan 5–12 V en VIN; consultar la documentación del módulo concreto.
- Durante las ráfagas de transmisión GSM el módem consume **picos de hasta 2 A**. Aunque este firmware solo usa GNSS, el módulo puede registrarse en la red si tiene SIM, y el soporte GSM/GPRS futuro sí transmitirá.
- **Alimentar el SIM808 desde una fuente independiente**, capaz de 2 A, con un capacitor de bajo ESR (≥ 470–1000 µF) cerca del módulo. No alimentarlo desde el pin 3V3 del Heltec ni desde el USB del ESP32: las caídas de tensión provocan reinicios del SIM808 y/o *brownout* del ESP32.
- **Nunca alimentar el SIM808 desde un GPIO del ESP32.**
- Unir el GND de la fuente del SIM808 con el GND del Heltec.

### Niveles lógicos UART

- La UART del chip SIM808 trabaja a **2.8 V** (VDD_EXT). Según el *hardware design* del SIM808, la entrada RXD admite como máximo ~3.1 V en alto.
- **SIM808 TXD → ESP32 RX**: 2.8 V en alto es reconocido por el ESP32 (VIH ≈ 2.5 V). Conexión directa.
- **ESP32 TX (3.3 V) → SIM808 RXD**: excede ligeramente el máximo del chip. Si el breakout **no** tiene adaptación de niveles, usar un adaptador (p. ej. BSS138) o un divisor resistivo (p. ej. 1 kΩ serie + 2.2 kΩ a GND ≈ 2.27 V). Muchos breakouts ya incluyen esta adaptación: verificar el esquema del módulo.

## OLED

| Parámetro | Valor |
| --------- | ----- |
| Controlador | SSD1306 |
| Resolución | 128 × 64 px, monocromo, 0.96" |
| Bus | I2C (bus `Wire`, 700 kHz) |
| Dirección I2C | `0x3C` |
| SDA | GPIO4 |
| SCL | GPIO15 |
| Reset | GPIO16 |
| Librería | [ThingPulse ESP8266 and ESP32 OLED driver for SSD1306](https://github.com/ThingPulse/esp8266-oled-ssd1306) (`SSD1306Wire`) |

Consideraciones del Heltec V2:

- La OLED **requiere un pulso de reset en GPIO16** antes de inicializarla; sin él la pantalla queda en negro. `displayInit()` lo hace.
- Los pines I2C de la OLED (4/15) no son los I2C por defecto del ESP32; se pasan explícitamente al constructor.
- `displayInit()` pone **Vext (GPIO21) en LOW** (salida de 3.3 V externa activa). Es inocuo si la OLED se alimenta directamente y garantiza el funcionamiento si depende de Vext.
- GPIO15 es pin de *strapping*; usarlo como SCL es el diseño original de Heltec y no afecta el arranque.
- Layout: título con fuente de 16 px y 4 líneas con fuente de 10 px.

## Software

- **PlatformIO** (VS Code + extensión PlatformIO IDE, o PlatformIO Core CLI).
- **Framework Arduino** sobre ESP32.
- Plataforma fijada: `espressif32@6.9.0`, que incluye **Arduino-ESP32 2.0.17**. Fijar la versión evita cambios de comportamiento por actualizaciones automáticas.
- Placa: `heltec_wifi_lora_32_V2`.
- Librerías:
  - `thingpulse/ESP8266 and ESP32 OLED driver for SSD1306 displays@^4.6.1` (OLED).
  - `HardwareSerial` y `Wire` del core Arduino-ESP32 (UART e I2C). Para el SIM808 no se usa ninguna librería externa: la capa AT propia es pequeña y deja control total de timeouts y errores.

### Estructura

```
heltec-sim808-gps/
├── platformio.ini
├── include/
│   ├── config.h      # Pines, baudrate, timeouts, DEBUG_AT
│   └── log.h         # Macros de log por Serial
├── src/
│   ├── main.cpp      # Máquina de estados de la aplicación
│   ├── sim808.h/.cpp # Capa AT: envío, espera, timeout, limpieza de buffer
│   ├── gps.h/.cpp    # GPSData, parser de +CGNSINF, control del GNSS
│   └── display.h/.cpp# Pantallas OLED
├── lib/
└── test/
```

## Instalación

```bash
git clone <url-del-repositorio>
cd heltec-sim808-gps
pio run                    # compilar
pio run --target upload    # cargar el firmware por USB
pio device monitor         # monitor serie a 115200 baud
```

En VS Code: abrir la carpeta del proyecto con la extensión PlatformIO instalada y usar los botones *Build*, *Upload* y *Monitor*.

En Windows puede ser necesario instalar el driver del puente USB-UART **CP210x** (Silicon Labs) que usa el Heltec V2.

## Funcionamiento

```
ESP32 inicia
      ↓
Inicializa OLED (reset GPIO16, I2C 4/15)
      ↓
Inicializa UART SIM808 (UART2, GPIO23/17, 9600 baud)
      ↓
Verifica SIM808 (AT, ATE0)            ── falla ─→ ERROR en OLED, reintento cada 5 s
      ↓
Activa GPS (AT+CGNSPWR? / AT+CGNSPWR=1) ── falla ─→ ERROR en OLED, reintento cada 5 s
      ↓
Espera GPS fix (AT+CGNSINF cada 2 s, muestra satélites y tiempo)
      ↓
Obtiene coordenadas y las valida
      ↓
Muestra Lat/Lon en OLED
      ↓
Repite (si se pierde el fix, vuelve a "Buscando fix...")
```

### Máquina de estados

| Estado | Acción | Transiciones |
| ------ | ------ | ------------ |
| `INIT` | `AT` (hasta 5 intentos) y `ATE0` | OK → `GPS_START`; falla → `GPS_ERROR` |
| `GPS_START` | Enciende y verifica el GNSS | OK → `GPS_SEARCHING`; falla → `GPS_ERROR` |
| `GPS_SEARCHING` | `AT+CGNSINF` cada 2 s | Fix → `GPS_FIXED`; GNSS apagado → `GPS_START`; 3 fallos AT seguidos → `GPS_ERROR` |
| `GPS_FIXED` | `AT+CGNSINF` cada 2 s, actualiza OLED | Sin fix → `GPS_SEARCHING`; mismas transiciones de error |
| `GPS_ERROR` | Muestra el error | Tras 5 s → `INIT` |

No hay `delay()` largos: la temporización usa `millis()`. Las únicas esperas son las de cada comando AT, acotadas por timeout (1–2 s), y el pulso opcional de PWRKEY (1.2 s).

### Comandos AT utilizados

| Comando | Uso |
| ------- | --- |
| `AT` | Verificar comunicación y sincronizar el autobaud |
| `ATE0` | Desactivar el eco |
| `AT+CGNSPWR?` | Consultar si el GNSS está encendido (`+CGNSPWR: <0/1>`) |
| `AT+CGNSPWR=1` | Encender el GNSS |
| `AT+CGNSINF` | Leer la información GNSS |

Formato de `+CGNSINF` (según *SIM800 Series GNSS Application Note* / manual AT del SIM808):

```
+CGNSINF: <run>,<fix>,<UTC>,<lat>,<lon>,<alt>,<speed>,<course>,<fixMode>,<res1>,
          <HDOP>,<PDOP>,<VDOP>,<res2>,<satsInView>,<satsUsed>,<glonassUsed>,<res3>,
          <C/N0max>,<HPA>,<VPA>
```

Ejemplo sin fix: `+CGNSINF: 1,0,,,,,,,0,,,,,,9,0,,,,,`
Ejemplo con fix: `+CGNSINF: 1,1,20261006130512.000,-27.469812,-58.830012,62.400,0.00,0.0,1,,1.1,1.4,0.9,,11,8,,,38,,`

Campos usados: `<run>` (1 = GNSS encendido), `<fix>` (1 = fix), `<lat>`/`<lon>` (grados decimales), `<HDOP>`, `<satsInView>` y `<satsUsed>`.

### Diferencias entre revisiones del SIM808

- Los firmwares **R14 y posteriores** (p. ej. `1418B0xSIM808M32`) implementan la familia **`AT+CGNS*`** usada en este proyecto.
- Firmwares más antiguos del SIM808 usan la familia **`AT+CGPS*`** (`AT+CGPSPWR`, `AT+CGPSSTATUS?`, `AT+CGPSINF`), con otro formato de respuesta (coordenadas en formato NMEA ddmm.mmmm). **Este firmware no la implementa**; con esos módulos el arranque del GPS fallará con `AT+CGNSPWR? not supported` en el monitor serie.
- Para conocer la revisión: enviar `AT+CGMR` (desde un terminal serie o agregándolo temporalmente). Los firmwares antiguos pueden actualizarse con la herramienta de SIMCom.

### Valores de `GPSData`

| Campo | Tipo | No disponible |
| ----- | ---- | ------------- |
| `valid` | `bool` | `false` (solo es `true` con fix y coordenadas validadas) |
| `latitude`, `longitude` | `double` | `NAN` |
| `satellites` (usados) | `int` | `-1` (`GPS_FIELD_UNAVAILABLE`) |
| `satellitesInView` | `int` | `-1` |
| `hdop` | `float` | `NAN` |

Validaciones: campos numéricos completos (no vacíos ni con basura), latitud en [-90, 90], longitud en [-180, 180] y descarte de (0, 0) exacto, que en la práctica indica datos inválidos.

### Depuración

En `include/config.h`:

```cpp
#define DEBUG_AT true   // false para ocultar el tráfico AT crudo
```

Salida típica:

```
[SIM808] Initializing...
[AT] >> AT
[AT] << OK
[SIM808] AT OK
[GPS] Starting GPS...
[GPS] GNSS powered on. Searching for fix...
[GPS] Searching for fix... (34 s, sats in view: 7, used: 0)
[GPS] FIX acquired after 52 s
[GPS] Latitude: -27.469812
[GPS] Longitude: -58.830012
```

## Cómo probar

1. Cablear según la tabla, con el SIM808 alimentado por su propia fuente y GND común.
2. Encender el SIM808 (LED de estado parpadeando) y cargar el firmware.
3. Abrir `pio device monitor`: debe verse `[SIM808] AT OK` y `[GPS] GNSS powered on`.
4. Colocar la antena GPS en exterior o junto a una ventana con cielo abierto.
5. La OLED muestra `GPS / Buscando fix...` con los satélites visibles. El primer fix en frío suele tardar 30 s a varios minutos.
6. Con fix, la OLED muestra `GPS FIX`, latitud, longitud, satélites y HDOP.
7. Prueba de robustez: desconectar el TX/RX del SIM808 → tras 3 fallos aparece `ERROR / SIM808 / Sin respuesta AT` y se reintenta cada 5 s; al reconectar se recupera solo.

## Troubleshooting

**SIM808 no responde** (`[SIM808] ERROR: no response to AT`)
- Verificar que el módulo esté encendido (LED de estado). Muchos requieren mantener el botón *PWR* ~1–2 s.
- Revisar el cruce TX/RX y el GND común.
- Revisar la alimentación (ver abajo).
- El autobaud se sincroniza con los primeros `AT`; si el módulo fue fijado a otro baudrate con `AT+IPR`, ajustar `SIM808_BAUD`.

**GPS nunca obtiene fix**
- La antena debe estar en el conector GPS/GNSS (no GSM) y con vista al cielo; dentro de edificios es muy difícil obtener fix.
- El primer fix (arranque en frío) puede tardar varios minutos. Si `Sat visibles` permanece en 0, revisar antena y conector.
- Algunas antenas activas necesitan alimentación del módulo; revisar la documentación del breakout.

**OLED no muestra información**
- Revisar en el monitor serie si aparece `[OLED] ERROR: init failed`.
- Confirmar que la placa sea V2 (la V3 usa ESP32-S3 y otros pines: SDA 17, SCL 18, RST 21).
- No reutilizar GPIO4, 15, 16 ni 21 para otras funciones.

**Coordenadas incorrectas**
- Con pocos satélites o HDOP alto (> 5) la posición es imprecisa; esperar a que mejore.
- Las coordenadas del SIM808 son WGS84 en grados decimales; negativas = Sur / Oeste.
- Una posición fija en 0,0 o fuera de rango es descartada por el firmware.

**UART no recibe datos**
- Con `DEBUG_AT true` deben verse las líneas `[AT] >> ...`. Si nunca aparece `[AT] << ...`, el problema es eléctrico (cableado, GND, niveles o módulo apagado).
- Verificar que nada más esté conectado a GPIO17/23.
- Respuestas con caracteres extraños indican baudrate incorrecto o ruido (cables largos).

**Problemas de alimentación**
- Síntomas: el SIM808 se reinicia (en el monitor aparece `GNSS reported OFF` o vuelve `RDY`), el LED de estado se apaga, o el ESP32 se reinicia con *brownout*.
- Usar una fuente de ≥ 2 A con capacitor de bulk cerca del módulo y cables cortos y gruesos.

**Problemas de niveles lógicos UART**
- El SIM808 trabaja a 2.8 V. Si el breakout no adapta niveles, colocar adaptador o divisor en la línea ESP32 TX → SIM808 RXD.
- No conectar módulos con UART a 5 V directamente al ESP32: los GPIO del ESP32 no toleran 5 V.

## Extensiones previstas

- **GSM/GPRS**: nuevo módulo que use `Sim808::sendCommand()` para `AT+CREG?`, `AT+SAPBR`/`AT+HTTP*` o `AT+CIP*`, compartiendo la UART con el GNSS.
- **LoRa/LoRaWAN**: el SX1276 de la placa usa pines que este firmware no toca (5, 14, 18, 19, 26, 27, 34, 35).
- **Envío de coordenadas**: `GPSData` ya contiene la última posición validada.
- Credenciales futuras (APN, claves LoRaWAN) en `include/secrets.h`, ya excluido por `.gitignore`.
