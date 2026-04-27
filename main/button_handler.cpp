#include "button_handler.h"
#include "device_state.h"
#include "storage.h"

#include <ESP8266WiFi.h>

static bool          handlerReady  = false;  // Set true only after buttonHandlerInit()
static bool          lastRawState  = HIGH;   // Last raw GPIO reading
static bool          stableState   = HIGH;   // Debounced state
static unsigned long debounceStart = 0;      // Timestamp of last raw transition
static unsigned long holdStart     = 0;      // Timestamp when stable LOW began
static bool          holdActive    = false;

void buttonHandlerInit() {
    // Flush any stale LOW that existed during boot
    lastRawState  = (digitalRead(PIN_BUTTON) == LOW) ? LOW : HIGH;
    stableState   = HIGH;   // Treat boot state as not-pressed regardless
    holdActive    = false;
    handlerReady  = true;
    Serial.println("[button] Handler active — monitoring GPIO 0");
}

void buttonHandlerLoop() {
    delay(1);
    if (!handlerReady) return;

    bool raw = (digitalRead(PIN_BUTTON) == LOW);   // true = pressed

    // ── Debounce ──────────────────────────────────────────────────────────────
    if (raw != lastRawState) {
        debounceStart = millis();
        lastRawState  = raw;
    }

    if ((millis() - debounceStart) < BUTTON_DEBOUNCE_MS) return;  // Unstable

    bool pressed = raw;   // Debounced value

    // ── Hold tracking ─────────────────────────────────────────────────────────
    if (pressed && !holdActive) {
        // Transition: released → pressed
        holdStart  = millis();
        holdActive = true;
        Serial.println("[button] Press started");
    } else if (!pressed && holdActive) {
        // Released before threshold
        holdActive = false;
        Serial.println("[button] Released early — timer reset");
    } else if (pressed && holdActive) {
        unsigned long held = millis() - holdStart;
        if (held >= BUTTON_HOLD_MS) {
            Serial.println("[button] 10-second hold confirmed — entering SETUP MODE");
            // Erase credentials so next boot enters setup
            storageErase();
            WiFi.disconnect();
            delay(300);
            ESP.restart();
        }
    }
}
