#include "button_handler.h"
#include "device_state.h"
#include "storage.h"

#include <ESP8266WiFi.h>


void buttonHandlerLoop() {
    bool reset = (digitalRead(PIN_BUTTON) == LOW);   // true = pressed

    if (reset) {
        Serial.println("[button] reset confirmed — entering SETUP MODE");
        // Erase credentials so next boot enters setup
        storageErase();
        WiFi.disconnect();
        delay(300);
        ESP.restart();
    }
}
