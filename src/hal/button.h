#pragma once

#include <Arduino.h>
#include <freertos/queue.h>

// Capa de abstracción de hardware para un pulsador.
//
// Una tarea FreeRTOS lee el pin cada `pollMs`, filtra rebotes y traduce las
// pulsaciones en eventos que se consumen desde loop() con getEvent(). Al no
// depender de loop(), no se pierden pulsaciones mientras loop() está bloqueado
// (p. ej. durante las ventanas RX de un uplink LoRaWAN).

enum class ButtonEvent : uint8_t {
    SinglePress,  // Una pulsación corta sin segunda pulsación dentro del gap
    DoublePress,  // Dos pulsaciones cortas seguidas
    LongPress,    // Mantenido `longPressMs` (se emite sin esperar a soltar)
};

const char *buttonEventName(ButtonEvent event);

struct ButtonTiming {
    uint32_t pollMs;
    uint32_t debounceMs;
    uint32_t clickMaxMs;       // Pulsaciones más largas no cuentan como clic
    uint32_t doublePressGapMs;
    uint32_t longPressMs;
};

class Button {
public:
    Button(int pin, bool activeLow, const ButtonTiming &timing);

    // Configura el pin y crea la tarea de lectura. Llamar una vez.
    bool begin();

    // Saca el próximo evento de la cola (no bloquea). false si no hay eventos.
    bool getEvent(ButtonEvent &event);

    // Tiempo que lleva presionado mientras una pulsación larga está en curso
    // (todavía no emitida). 0 si no está presionado o ya se emitió LongPress.
    uint32_t holdMs() const;

private:
    static void taskEntry(void *arg);
    void poll();
    void onPress(uint32_t now);
    void onRelease(uint32_t now);
    void emit(ButtonEvent event);
    bool readPressed() const;

    const int pin_;
    const bool activeLow_;
    const ButtonTiming timing_;
    QueueHandle_t queue_ = nullptr;

    // Estado de la tarea de lectura. Los volatile se leen desde holdMs().
    bool lastRaw_ = false;
    uint32_t lastChangeMs_ = 0;
    volatile bool pressed_ = false;  // Estado filtrado
    volatile bool longFired_ = false;
    volatile uint32_t pressStartMs_ = 0;
    uint32_t releaseMs_ = 0;
    uint8_t clicks_ = 0;
};
