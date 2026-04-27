#include "setup_mode.h"
#include "device_state.h"
#include "storage.h"

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

static ESP8266WebServer setupServer(80);
static const IPAddress AP_IP(192, 168, 4, 1);
static const IPAddress AP_SUBNET(255, 255, 255, 0);

// ── HTML pages ────────────────────────────────────────────────────────────────

static const char PAGE_FORM[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Smart Device Setup</title>
  <style>
    *{box-sizing:border-box;margin:0;padding:0}
    body{font-family:sans-serif;background:#f0f4f8;display:flex;align-items:center;
         justify-content:center;min-height:100vh}
    .card{background:#fff;border-radius:12px;padding:32px;width:100%;max-width:380px;
          box-shadow:0 4px 24px rgba(0,0,0,.1)}
    h1{font-size:1.3rem;color:#333;margin-bottom:8px}
    p.sub{font-size:.85rem;color:#888;margin-bottom:24px}
    label{display:block;font-size:.85rem;color:#555;margin-bottom:4px}
    input{width:100%;padding:10px 12px;border:1px solid #ddd;border-radius:8px;
          font-size:.95rem;margin-bottom:16px;outline:none}
    input:focus{border-color:#4a90e2}
    button{width:100%;padding:12px;background:#4a90e2;color:#fff;border:none;
           border-radius:8px;font-size:1rem;cursor:pointer}
    button:hover{background:#357abd}
    .err{background:#fdecea;color:#c0392b;padding:10px 14px;border-radius:8px;
         margin-bottom:16px;font-size:.88rem}
  </style>
</head>
<body>
<div class="card">
  <h1>&#128267; Device Setup</h1>
  <p class="sub">Connect this device to your Wi-Fi network.</p>
  %ERROR%
  <form method="POST" action="/configure">
    <label for="name">Device Name</label>
    <input id="name" name="name" type="text" placeholder="e.g. Living Room Relay" required maxlength="32">
    <label for="ssid">Wi-Fi Network (SSID)</label>
    <input id="ssid" name="ssid" type="text" placeholder="Your network name" required maxlength="32">
    <label for="password">Wi-Fi Password</label>
    <input id="password" name="password" type="password" placeholder="Leave blank if open" maxlength="64">
    <button type="submit">Connect</button>
  </form>
</div>
</body>
</html>
)rawhtml";

static const char PAGE_SUCCESS[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Connected!</title>
  <style>
    body{font-family:sans-serif;background:#f0f4f8;display:flex;align-items:center;
         justify-content:center;min-height:100vh}
    .card{background:#fff;border-radius:12px;padding:40px;text-align:center;
          box-shadow:0 4px 24px rgba(0,0,0,.1)}
    .icon{font-size:3rem}
    h1{color:#27ae60;margin:16px 0 8px}
    p{color:#666;font-size:.9rem}
  </style>
</head>
<body>
<div class="card">
  <div class="icon">&#10003;</div>
  <h1>Connected!</h1>
  <p>Device is restarting and will join your network shortly.</p>
</div>
</body>
</html>
)rawhtml";

// ── Handler helpers ───────────────────────────────────────────────────────────

static void serveForm(const String& errorMsg = "") {
    String page = FPSTR(PAGE_FORM);
    if (errorMsg.length() > 0) {
        page.replace("%ERROR%",
            "<div class=\"err\">&#10007; " + errorMsg + "</div>");
    } else {
        page.replace("%ERROR%", "");
    }
    setupServer.send(200, "text/html", page);
}

static void handleRoot() {
    serveForm();
}

static void handleConfigure() {
    String newName     = setupServer.arg("name");
    String newSsid     = setupServer.arg("ssid");
    String newPassword = setupServer.arg("password");

    newName.trim(); newSsid.trim();

    if (newSsid.isEmpty()) {
        serveForm("SSID cannot be empty.");
        return;
    }
    if (newName.isEmpty()) {
        serveForm("Device name cannot be empty.");
        return;
    }

    Serial.println("[setup_mode] Trying SSID: " + newSsid);
    Serial.println("[setup_mode] Password: " + newPassword);
    Serial.println("[setup_mode] device name: " + newName);

    // Try connecting (AP+STA simultaneously so portal stays up)
    WiFi.mode(WIFI_AP_STA);
    WiFi.begin(newSsid.c_str(), newPassword.c_str());

    const unsigned long timeout = 18000UL;  // 18 s
    unsigned long t0 = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - t0) < timeout) {
        setupServer.handleClient();         // Keep portal responsive while waiting
        delay(200);
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[setup_mode] Connection failed");
        WiFi.disconnect();
        WiFi.mode(WIFI_AP);
        serveForm("Could not connect. Please check your credentials.");
        return;
    }

    // ── Success ───────────────────────────────────────────────────────────────
    Serial.println("[setup_mode] Connected — IP: " + WiFi.localIP().toString());

    gDevice.name     = newName;
    gDevice.ssid     = newSsid;
    gDevice.password = newPassword;
    storageWrite();

    setupServer.send(200, "text/html", FPSTR(PAGE_SUCCESS));
    delay(2000);

    WiFi.softAPdisconnect(true);
    Serial.println("[setup_mode] Restarting…");
    delay(500);
    ESP.restart();
}

// ── Public API ────────────────────────────────────────────────────────────────

void setupModeBegin() {
    Serial.println("[setup_mode] Entering SETUP MODE");

    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(AP_IP, AP_IP, AP_SUBNET);

    // Derive AP SSID from deviceId prefix or use generic name
    String apSsid = "SmartDevice-" + gDevice.deviceId.substring(0,
                     gDevice.deviceId.indexOf('-'));
    if (apSsid == "SmartDevice-") apSsid = "SmartDevice-Setup";

    WiFi.softAP(apSsid.c_str());   // Open network — no password
    Serial.println("[setup_mode] AP: " + apSsid + " @ " + AP_IP.toString());

    setupServer.on("/",          HTTP_GET,  handleRoot);
    setupServer.on("/configure", HTTP_POST, handleConfigure);
    setupServer.onNotFound([](){ setupServer.sendHeader("Location", "/"); setupServer.send(302); });
    setupServer.begin();
    Serial.println("[setup_mode] HTTP server started on port 80");

    // Block here — we only leave via ESP.restart() on success
    while (true) {
        setupServer.handleClient();
        yield();
    }
}
