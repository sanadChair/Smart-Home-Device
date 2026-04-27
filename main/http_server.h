#pragma once

#include <Arduino.h>

// ── HTTP Server ───────────────────────────────────────────────────────────────
// Exposes two endpoints in OPERATIONAL MODE:
//
//   GET  /status?deviceId=<id>   → 200 JSON descriptor | 403
//   POST /control                → 200 JSON descriptor | 400 | 403
//
// Self-heals: if the server object enters a bad state the caller can
// restart it via httpServerBegin().

// Call once after Wi-Fi is connected.
void httpServerBegin();

// Call every loop() iteration.
void httpServerLoop();
