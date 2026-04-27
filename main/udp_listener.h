#pragma once

#include <Arduino.h>

// ── UDP Listener ──────────────────────────────────────────────────────────────
// Listens on UDP port 9988 for any discovery broadcast.
// Replies to the sender with a JSON device descriptor.
// Self-heals: restarts the socket if it enters a failed state.

constexpr uint16_t UDP_LISTEN_PORT = 9988;

// Call once after Wi-Fi is connected.
void udpListenerBegin();

// Call every loop() iteration to service incoming packets.
void udpListenerLoop();
