#include "button.h"

#include "log.h"

namespace {

constexpr UBaseType_t BUTTON_QUEUE_LENGTH = 8;
constexpr uint32_t BUTTON_TASK_STACK_BYTES = 2048;
// Prioridad mayor que loop() (1) para seguir leyendo aunque loop() haga esperas activas.
constexpr UBaseType_t BUTTON_TASK_PRIORITY = 2;

}  // namespace

const char *buttonEventName(ButtonEvent event) {
    switch (event) {
        case ButtonEvent::SinglePress: return "SINGLE";
        case ButtonEvent::DoublePress: return "DOUBLE";
        case ButtonEvent::LongPress: return "LONG";
    }
    return "?";
}

Button::Button(int pin, bool activeLow, const ButtonTiming &timing)
    : pin_(pin), activeLow_(activeLow), timing_(timing) {}

bool Button::begin() {
    pinMode(pin_, activeLow_ ? INPUT_PULLUP : INPUT_PULLDOWN);
    lastRaw_ = readPressed();
    pressed_ = lastRaw_;
    // Si arranca presionado, ignorar esa pulsación hasta que se suelte.
    longFired_ = lastRaw_;

    queue_ = xQueueCreate(BUTTON_QUEUE_LENGTH, sizeof(ButtonEvent));
    if (!queue_ ||
        xTaskCreate(taskEntry, "button", BUTTON_TASK_STACK_BYTES, this, BUTTON_TASK_PRIORITY, nullptr) != pdPASS) {
        LOG("BTN", "ERROR: could not create button task");
        return false;
    }
    LOG("BTN", "Button on GPIO%d ready", pin_);
    return true;
}

bool Button::getEvent(ButtonEvent &event) {
    return queue_ && xQueueReceive(queue_, &event, 0) == pdTRUE;
}

uint32_t Button::holdMs() const {
    if (!pressed_ || longFired_) {
        return 0;
    }
    return millis() - pressStartMs_;
}

void Button::taskEntry(void *arg) {
    Button *self = static_cast<Button *>(arg);
    for (;;) {
        self->poll();
        vTaskDelay(pdMS_TO_TICKS(self->timing_.pollMs));
    }
}

bool Button::readPressed() const {
    return (digitalRead(pin_) == LOW) == activeLow_;
}

void Button::poll() {
    const uint32_t now = millis();
    const bool raw = readPressed();

    // Antirrebote: el estado se acepta cuando se mantiene estable debounceMs.
    if (raw != lastRaw_) {
        lastRaw_ = raw;
        lastChangeMs_ = now;
    }
    if (raw != pressed_ && now - lastChangeMs_ >= timing_.debounceMs) {
        if (raw) {
            onPress(now);
        } else {
            onRelease(now);
        }
    }

    if (pressed_ && !longFired_ && now - pressStartMs_ >= timing_.longPressMs) {
        longFired_ = true;
        clicks_ = 0;
        emit(ButtonEvent::LongPress);
    }

    if (!pressed_ && clicks_ == 1 && now - releaseMs_ > timing_.doublePressGapMs) {
        clicks_ = 0;
        emit(ButtonEvent::SinglePress);
    }
}

void Button::onPress(uint32_t now) {
    pressStartMs_ = now;
    longFired_ = false;
    pressed_ = true;
}

void Button::onRelease(uint32_t now) {
    pressed_ = false;
    if (longFired_) {
        clicks_ = 0;  // Fin de una pulsación larga ya emitida
        return;
    }
    if (now - pressStartMs_ > timing_.clickMaxMs) {
        clicks_ = 0;  // Pulsación larga abortada: no cuenta como clic
        return;
    }
    releaseMs_ = now;
    if (++clicks_ >= 2) {
        clicks_ = 0;
        emit(ButtonEvent::DoublePress);
    }
}

void Button::emit(ButtonEvent event) {
    LOG("BTN", "%s", buttonEventName(event));
    if (xQueueSend(queue_, &event, 0) != pdTRUE) {
        LOG("BTN", "WARN: event queue full, %s dropped", buttonEventName(event));
    }
}
