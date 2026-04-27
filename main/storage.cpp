#include "storage.h"
#include "device_state.h"

#include <LittleFS.h>
#include <ArduinoJson.h>

static const char* CONFIG_PATH = "/config.json";

// ── Internal helpers ──────────────────────────────────────────────────────────

static bool mountFS() {
    if (!LittleFS.begin()) {
        Serial.println("[storage] LittleFS mount failed — formatting");
        LittleFS.format();
        if (!LittleFS.begin()) {
            Serial.println("[storage] LittleFS mount failed after format");
            return false;
        }
    }
    return true;
}

static void applyDefaults() {
    // Keep whatever compile-time constant the constructor set; zero everything else.
    gDevice.name        = "";
    gDevice.ssid        = "";
    gDevice.password    = "";
    gDevice.controlType = CONTROL_TYPE;
    gDevice.state       = 0;
    // deviceId is preserved (set by DeviceState constructor).
}

// ── Public API ────────────────────────────────────────────────────────────────

bool storageInit() {
    if (!mountFS()) {
        applyDefaults();
        return false;
    }

    // ── Read existing config ──────────────────────────────────────────────────
    if (LittleFS.exists(CONFIG_PATH)) {
        File f = LittleFS.open(CONFIG_PATH, "r");
        if (!f) {
            Serial.println("[storage] Cannot open config — applying defaults");
            applyDefaults();
            return false;
        }

        StaticJsonDocument<512> doc;
        DeserializationError err = deserializeJson(doc, f);
        f.close();

        if (err) {
            Serial.println("[storage] JSON parse error: " + String(err.c_str()));
            applyDefaults();
            return false;
        }

        gDevice.deviceId    = doc["deviceId"]    | String(DEVICE_ID_CONSTANT);
        gDevice.name        = doc["name"]         | String("");
        gDevice.ssid        = doc["ssid"]         | String("");
        gDevice.password    = doc["password"]     | String("");
        gDevice.controlType = doc["controlType"]  | String(CONTROL_TYPE);
        gDevice.state       = doc["state"]        | 0;

        Serial.println("[storage] Loaded from flash — ssid: " + gDevice.ssid
                       + " | name: " + gDevice.name
                       + " | state: " + String(gDevice.state));
    } else {
        Serial.println("[storage] No config file — applying defaults");
        applyDefaults();
    }

    // ── Guarantee deviceId is persisted ──────────────────────────────────────
    if (gDevice.deviceId.isEmpty()) {
        gDevice.deviceId = DEVICE_ID_CONSTANT;
        Serial.println("[storage] deviceId missing — writing compile-time constant");
        storageWrite();
    }

    return true;
}

bool storageWrite() {
    File f = LittleFS.open(CONFIG_PATH, "w");
    if (!f) {
        Serial.println("[storage] Cannot open config for writing");
        return false;
    }

    StaticJsonDocument<512> doc;
    doc["deviceId"]    = gDevice.deviceId;
    doc["name"]        = gDevice.name;
    doc["ssid"]        = gDevice.ssid;
    doc["password"]    = gDevice.password;
    doc["controlType"] = gDevice.controlType;
    doc["state"]       = gDevice.state;

    serializeJson(doc, f);
    f.close();

    Serial.println("[storage] Config written to flash");
    return true;
}

void storageErase() {
    if (LittleFS.exists(CONFIG_PATH)) {
        LittleFS.remove(CONFIG_PATH);
        Serial.println("[storage] Config erased");
    }
}
