#include "http_server.h"
#include "device_state.h"

#include <ESP8266WebServer.h>
#include <ArduinoJson.h>

static ESP8266WebServer server(80);

// ── Helpers ───────────────────────────────────────────────────────────────────

static String buildDescriptor() {
    StaticJsonDocument<256> doc;
    doc["deviceId"]    = gDevice.deviceId;
    doc["name"]        = gDevice.name;
    doc["controlType"] = gDevice.controlType;
    doc["state"]       = gDevice.state;
    String out;
    serializeJson(doc, out);
    return out;
}

static void sendJson(int code, const String& body) {
    server.send(code, "application/json", body);
}

// ── GET /status ───────────────────────────────────────────────────────────────

static void handleStatus() {
    String requestedId = server.arg("deviceId");
    if (requestedId != gDevice.deviceId) {
        Serial.println("[http] /status — deviceId mismatch: " + requestedId);
        sendJson(403, "{\"error\":\"Device ID mismatch\"}");
        return;
    }
    Serial.println("[http] /status → 200");
    sendJson(200, buildDescriptor());
}

// ── POST /control ─────────────────────────────────────────────────────────────

static void handleControl() {
    if (!server.hasArg("plain") || server.arg("plain").isEmpty()) {
        sendJson(400, "{\"error\":\"Empty body\"}");
        return;
    }

    StaticJsonDocument<128> doc;
    DeserializationError err = deserializeJson(doc, server.arg("plain"));
    if (err) {
        Serial.println("[http] /control — JSON parse error: " + String(err.c_str()));
        sendJson(400, "{\"error\":\"Invalid JSON: " + String(err.c_str()) + "\"}");
        return;
    }

    // Validate deviceId
    String requestedId = doc["deviceId"] | String("");
    if (requestedId != gDevice.deviceId) {
        Serial.println("[http] /control — deviceId mismatch: " + requestedId);
        sendJson(403, "{\"error\":\"Device ID mismatch\"}");
        return;
    }

    // Validate state
    if (!doc.containsKey("state") || !doc["state"].is<int>()) {
        sendJson(400, "{\"error\":\"Missing or non-integer 'state' field\"}");
        return;
    }
    int newState = doc["state"].as<int>();
    if (newState != 0 && newState != 1) {
        sendJson(400, "{\"error\":\"Invalid state value. Must be 0 or 1.\"}");
        return;
    }

    deviceStateSet(newState);   // Updates memory, persists, drives GPIO
    Serial.printf("[http] /control → state=%d  200\n", newState);
    sendJson(200, buildDescriptor());
}

// ── 404 catch-all ─────────────────────────────────────────────────────────────

static void handleNotFound() {
    sendJson(404, "{\"error\":\"Not found\"}");
}

// ── Public API ────────────────────────────────────────────────────────────────

void httpServerBegin() {
    server.on("/status",  HTTP_GET,  handleStatus);
    server.on("/control", HTTP_POST, handleControl);
    server.onNotFound(handleNotFound);
    server.begin();
    Serial.println("[http] Server started on port 80");
}

void httpServerLoop() {
    delay(1);
    server.handleClient();
}
