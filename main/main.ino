// ── ESP8266 Smart Home Device — main.cpp ─────────────────────────────────────
// Boot flow:
//   1. storageInit()          Load/create persistent config (deviceId guaranteed)
//   2. deviceStateInitGPIO()  Configure GPIO 0 (button) + GPIO 2 (relay)
//   3. If no SSID → setupModeBegin()  [blocks until provisioned then restarts]
//   4. wifiManagerConnect()   STA connection (restarts on total failure)
//   5. udpListenerBegin()     Bind UDP port 9988
//   6. httpServerBegin()      Register REST endpoints on port 80
//   7. buttonHandlerInit()    Arm the 10-second hold detector (post-boot only)
//   loop(): service UDP, HTTP, button, maintain Wi-Fi

#include <Arduino.h>

#include "device_state.h"
#include "storage.h"
#include "setup_mode.h"
#include "wifi_manager.h"
#include "udp_listener.h"
#include "http_server.h"
#include "button_handler.h"

// ── setup() ───────────────────────────────────────────────────────────────────

void setup() {
    Serial.begin(115200);
    delay(100);
    Serial.println("\n\n[main] ── Boot ──────────────────────────────");

    // ── 1. Persistent storage ─────────────────────────────────────────────────
    bool storageOk = storageInit();
    if (!storageOk) {
        Serial.println("[main] Storage error — running with defaults");
    }
    Serial.println("[main] deviceId : " + gDevice.deviceId);
    Serial.println("[main] name     : " + gDevice.name);
    Serial.println("[main] ssid     : " + gDevice.ssid);
    Serial.println("[main] state    : " + String(gDevice.state));

    // ── 2. GPIO initialisation ────────────────────────────────────────────────
    deviceStateInitGPIO();

    // ── 3. Mode selection ─────────────────────────────────────────────────────
    if (gDevice.ssid.isEmpty()) {
        Serial.println("[main] No SSID stored — entering SETUP MODE");
        setupModeBegin();   // Does not return; reboots on success
    }

    // ── 4. Wi-Fi connection (STA) ─────────────────────────────────────────────
    Serial.println("[main] Entering OPERATIONAL MODE");
    wifiManagerConnect();   // Blocks; restarts device on failure

    // ── 5. UDP discovery listener ─────────────────────────────────────────────
    udpListenerBegin();

    // ── 6. HTTP REST server ───────────────────────────────────────────────────
    httpServerBegin();

    // ── 7. Button handler — MUST be last ─────────────────────────────────────
    // Arming here ensures any LOW on GPIO 0 during boot is ignored.
    buttonHandlerInit();

    Serial.println("[main] ── Ready ──────────────────────────────");
}

// ── loop() ────────────────────────────────────────────────────────────────────

static unsigned long lastWifiCheck = 0;
constexpr unsigned long WIFI_CHECK_INTERVAL = 15000UL;  // Check every 15 s

void loop() {
    // Wi-Fi health check (non-blocking; only reconnects when needed)
    if (millis() - lastWifiCheck >= WIFI_CHECK_INTERVAL) {
        lastWifiCheck = millis();
        if (!wifiManagerMaintain()) {
            // Wi-Fi lost and reconnect attempt failed — log and continue.
            // UDP and HTTP will naturally recover once Wi-Fi is back.
            Serial.println("[main] Wi-Fi unavailable — will retry");
        }
    }

    udpListenerLoop();
    httpServerLoop();
    buttonHandlerLoop();
}
