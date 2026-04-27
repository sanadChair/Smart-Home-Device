#pragma once

#include <Arduino.h>

// ── Setup Mode Module ─────────────────────────────────────────────────────────
// Starts a Wi-Fi Access Point with an HTTP configuration portal.
// User submits credentials; module attempts connection and, on success,
// persists them and reboots into OPERATIONAL MODE.

// Start AP + HTTP server.  Blocks inside a poll loop until provisioned.
void setupModeBegin();
