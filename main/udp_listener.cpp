#include "udp_listener.h"
#include "device_state.h"

#include <WiFiUdp.h>
#include <ArduinoJson.h>

static WiFiUDP udp;
static bool    udpReady = false;

static bool startSocket() {
    udpReady = (udp.begin(UDP_LISTEN_PORT) == 1);
    if (udpReady) {
        Serial.printf("[udp] Listening on port %u\n", UDP_LISTEN_PORT);
    } else {
        Serial.println("[udp] Failed to bind socket");
    }
    return udpReady;
}

void udpListenerBegin() {
    startSocket();
}
void udpListenerLoop() {
    // Auto-heal if socket dropped
    delay(1);
    if (!udpReady) {
        Serial.println("[udp] Restarting socket");
        startSocket();
        return;
    }

    int packetSize = udp.parsePacket();
    if (packetSize <= 0) return;  // Nothing arrived

    IPAddress remoteIp   = udp.remoteIP();
    uint16_t  remotePort = udp.remotePort();

    // Read incoming payload
    char buf[128] = {0};
    int len = 0;
    while (udp.available() && len < (int)sizeof(buf) - 1) {
        buf[len++] = udp.read();
    }

    // Parse requestId from incoming payload
    StaticJsonDocument<128> incoming;
    int requestId = -1;
    if (!deserializeJson(incoming, buf)) {
        requestId = incoming["requestId"] | -1;
    }

    Serial.printf("[udp] Discovery from %s:%u — sending descriptor\n",
                  remoteIp.toString().c_str(), remotePort);

    // Build JSON response
    StaticJsonDocument<256> doc;
    doc["type"]        = "discovery:response";
    doc["requestId"]   = requestId;
    doc["deviceId"]    = gDevice.deviceId;
    doc["name"]        = gDevice.name;
    doc["controlType"] = gDevice.controlType;
    doc["state"]       = gDevice.state;

    String payload;
    serializeJson(doc, payload);

    // Reply only to the originating sender
    if (udp.beginPacket(remoteIp, remotePort)) {
        udp.write(payload.c_str(), payload.length());
        if (!udp.endPacket()) {
            Serial.println("[udp] Send failed");
            udpReady = false;   // Force socket restart next loop
        }
    } else {
        Serial.println("[udp] beginPacket failed");
        udpReady = false;
    }
}
