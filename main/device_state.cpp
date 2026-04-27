#include "device_state.h"
#include "storage.h"

DeviceState gDevice;

void deviceStateInitGPIO() {
    pinMode(PIN_BUTTON, INPUT_PULLUP);          // External or internal pullup via board
    pinMode(PIN_RELAY,  OUTPUT);
    deviceStateApplyOutput();
}

void deviceStateApplyOutput() {
    digitalWrite(PIN_RELAY, gDevice.state == 0 ? HIGH : LOW);
}

bool deviceStateSet(int newState) {
    if (newState != 0 && newState != 1) {
        Serial.println("[device_state] Invalid state value: " + String(newState));
        return false;
    }
    gDevice.state = newState;
    storageWrite();            // Persist immediately
    deviceStateApplyOutput();  // Drive GPIO
    Serial.println("[device_state] State → " + String(newState));
    return true;
}
