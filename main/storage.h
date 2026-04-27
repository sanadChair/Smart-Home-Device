#pragma once

#include <Arduino.h>

// ── Storage Module ────────────────────────────────────────────────────────────
// Reads and writes all persistent fields to LittleFS (JSON file).
// Falls back gracefully on corruption: logs error, applies defaults.
//
// Must be the FIRST module initialised on every boot.

// Mount LittleFS, load /config.json into gDevice.
// If deviceId is absent or empty in storage, writes the compile-time constant.
// Returns true on clean load, false on error (defaults applied).
bool storageInit();

// Serialise gDevice → /config.json.
// Returns true on success.
bool storageWrite();

// Wipe /config.json (used for factory-reset / forced re-provisioning).
void storageErase();
