#pragma once

#include <Arduino.h>

// ── Wi-Fi Manager ─────────────────────────────────────────────────────────────
// Connects to the stored SSID/password in STA mode.
// Retries up to MAX_RETRIES times; restarts device on total failure.
// Call once from main boot sequence after storageInit().

constexpr int   WIFI_MAX_RETRIES       = 5;
constexpr unsigned long WIFI_RETRY_MS  = 5000UL;
constexpr unsigned long WIFI_ATTEMPT_MS = 10000UL; // per attempt timeout

// Blocks until connected or retries exhausted (triggers restart on failure).
void wifiManagerConnect();

// Non-blocking reconnect check — call from main loop to handle drops.
// Returns true if already connected; attempts one reconnect step if not.
bool wifiManagerMaintain();
