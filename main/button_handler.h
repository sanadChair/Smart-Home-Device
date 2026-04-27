#pragma once

#include <Arduino.h>

// ── Button Handler ────────────────────────────────────────────────────────────
// Monitors GPIO 0 (active LOW) for a sustained 10-second hold.
// Debounce: 50 ms minimum stable LOW before the hold counter increments.
// Safety: evaluation only starts after setup() completes — avoids
//         misinterpreting the boot-time flash-mode LOW as a hold.
//
// On confirmed 10-second hold: erases stored credentials and restarts
// into SETUP MODE automatically.

constexpr unsigned long BUTTON_HOLD_MS    = 3000UL;
constexpr unsigned long BUTTON_DEBOUNCE_MS = 50UL;

// Call once at the END of setup() (after all modules are initialised).
void buttonHandlerInit();

// Call every loop() iteration.
void buttonHandlerLoop();
