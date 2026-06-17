#include "wifi_manager.h"
#include "setup_mode.h"
#include "device_state.h"
#include "button_handler.h"
#include <ESP8266WiFi.h>

static bool attemptConnect() {
    Serial.println("[wifi] Connecting to: " + gDevice.ssid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(gDevice.ssid.c_str(), gDevice.password.c_str());

    unsigned long t0 = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - t0 > WIFI_ATTEMPT_MS) {
            return false;
        }
        delay(250);
        Serial.print(".");
    }
    Serial.println();
    return true;
}

void wifiManagerConnect() {
    for (int attempt = 1; attempt <= WIFI_MAX_RETRIES; attempt++) {
        buttonHandlerLoop();
        Serial.printf("[wifi] Attempt %d / %d\n", attempt, WIFI_MAX_RETRIES);
        if (attemptConnect()) {
            Serial.println("[wifi] Connected — IP: " + WiFi.localIP().toString());
            return;
        }
        Serial.println("[wifi] Attempt failed");
        WiFi.disconnect();
        if (attempt < WIFI_MAX_RETRIES) {
            Serial.printf("[wifi] Waiting %lu ms before retry\n", WIFI_RETRY_MS);
            delay(WIFI_RETRY_MS);
        }
    }

    // All retries exhausted — restart (do NOT fall back to setup mode;
    // the failure may be transient per §5.1).
    Serial.println("[wifi] All retries failed — entering setup mode");
    delay(500);
    setupModeBegin();
}

bool wifiManagerMaintain() {
    if (WiFi.status() == WL_CONNECTED) return true;

    Serial.println("[wifi] Connection lost — attempting reconnect");
    WiFi.disconnect();
    delay(500);

    if (attemptConnect()) {
        Serial.println("[wifi] Reconnected — IP: " + WiFi.localIP().toString());
        return true;
    }

    Serial.println("[wifi] Reconnect failed — will retry next cycle");
    return false;
}
