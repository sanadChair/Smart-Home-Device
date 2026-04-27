#pragma once

#include <Arduino.h>

// ── Compile-time device identity ──────────────────────────────────────────────
// Change this before flashing each unique unit.
#define DEVICE_ID_CONSTANT "relay-ct2112rx1"
#define CONTROL_TYPE       "binary"

// ── GPIO pin assignments (ESP-01) ─────────────────────────────────────────────
constexpr uint8_t PIN_BUTTON = 0;   // Active LOW, internal pullup, boot-critical
constexpr uint8_t PIN_RELAY  = 2;   // Active HIGH; mirrors onboard LED

// ── DeviceState ───────────────────────────────────────────────────────────────
// Holds the authoritative in-memory copy of all runtime fields.
// Storage layer reads into this on boot; every other module reads from it.

struct DeviceState {
    String deviceId;
    String name;
    String ssid;
    String password;
    String controlType;
    int    state;       // 0 = off, 1 = on

    DeviceState()
        : deviceId(DEVICE_ID_CONSTANT),
          controlType(CONTROL_TYPE),
          state(0) {}
};

// Global singleton — defined in device_state.cpp
extern DeviceState gDevice;

// ── GPIO helpers ──────────────────────────────────────────────────────────────

// Call once from setup() after storage is loaded.
void deviceStateInitGPIO();

// Apply gDevice.state to GPIO 2 (and therefore the relay / LED).
void deviceStateApplyOutput();

// Set state, persist it, and drive GPIO atomically.
// Returns false only if state value is invalid (not 0 or 1).
bool deviceStateSet(int newState);
