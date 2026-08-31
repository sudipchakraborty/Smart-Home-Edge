#include "WiFiSetupServer.h"
#include <WiFi.h>

namespace {
const char SETUP_PAGE[] PROGMEM = R"HTML(
<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Device WiFi Setup</title><style>:root{font-family:system-ui,sans-serif;color:#18212f;background:#eef2f7}*{box-sizing:border-box}body{margin:0;min-height:100vh;display:grid;place-items:center;padding:20px}.box{width:min(520px,100%);background:#fff;border-radius:18px;padding:26px;box-shadow:0 16px 45px #1c304426}h1{margin:0 0 5px}.sub{color:#627086;margin:0 0 24px}.info{background:#edf7ff;border-radius:10px;padding:12px;margin-bottom:18px;color:#265476}label{display:block;font-weight:650;margin:14px 0 6px}input{width:100%;padding:12px;border:1px solid #bdc8d6;border-radius:9px;font-size:1rem}button{width:100%;border:0;border-radius:9px;padding:13px;margin-top:20px;background:#1777d2;color:#fff;font-weight:700;font-size:1rem;cursor:pointer}button:disabled{opacity:.6}.status{min-height:24px;text-align:center;margin-top:14px;color:#1777d2}.links{text-align:center;margin-top:14px}.links a{color:#1777d2}</style></head>
<body><main class="box"><h1>WiFi Setup</h1><p class="sub">Connect this device to your local 2.4 GHz WiFi network.</p><div class="info" id="device">Loading device status...</div><form id="form"><label for="ssid">WiFi network name (SSID)</label><input id="ssid" name="ssid" maxlength="32" autocomplete="off" required><label for="password">WiFi password</label><input id="password" name="password" type="password" maxlength="63" autocomplete="new-password"><button id="save" type="submit">Save and connect</button><div class="status" id="status"></div></form><div class="links"><a href="http://192.168.4.1:8080/">Relay timer settings</a></div></main>
<script>const msg=document.querySelector('#status'),btn=document.querySelector('#save'),info=document.querySelector('#device');async function status(){try{const r=await fetch('/api/status'),d=await r.json();info.textContent='Device: '+d.serial+' | Setup hotspot: '+d.ap;if(d.connected)msg.textContent='Connected to '+d.ssid+' | IP '+d.ip;else if(d.connecting)msg.textContent='Connecting...';return d}catch(e){return null}}document.querySelector('#form').addEventListener('submit',async e=>{e.preventDefault();btn.disabled=true;msg.textContent='Saving...';try{const r=await fetch('/api/wifi',{method:'POST',body:new FormData(e.target)}),d=await r.json();if(!r.ok)throw new Error(d.message);msg.textContent=d.message;setTimeout(poll,1000)}catch(e){msg.textContent=e.message;btn.disabled=false}});async function poll(){const d=await status();if(d&&d.connecting)setTimeout(poll,1000);else btn.disabled=false}status();</script></body></html>)HTML";
}

WiFiSetupServer::WiFiSetupServer(uint16_t port) : _server(port), _connectPending(false), _connectStartedAt(0) {}

bool WiFiSetupServer::begin(const String &hotspotName,
                            const String &hotspotPassword,
                            const String &serialNumber) {
    _serialNumber = serialNumber;
    _accessPointName = hotspotName;
    _accessPointPassword = hotspotPassword;
    if (_accessPointName.length() > 32) _accessPointName.remove(32);
    if (_accessPointName.isEmpty() || _accessPointPassword.length() < 8 ||
        _accessPointPassword.length() > 63) return false;
    WiFi.mode(WIFI_AP_STA);
    if (!WiFi.softAP(_accessPointName.c_str(), _accessPointPassword.c_str())) return false;
    _dns.start(53, "*", WiFi.softAPIP());
    _server.on("/", HTTP_GET, [this]() { handleRoot(); });
    _server.on("/api/status", HTTP_GET, [this]() { handleStatus(); });
    _server.on("/api/wifi", HTTP_POST, [this]() { handleSave(); });
    _server.onNotFound([this]() { handleRoot(); });
    _server.begin();
    return true;
}

void WiFiSetupServer::handleClient() {
    _dns.processNextRequest();
    _server.handleClient();
    if (_connectPending && (WiFi.status() == WL_CONNECTED || millis() - _connectStartedAt >= 20000UL)) _connectPending = false;
}

String WiFiSetupServer::accessPointName() const { return _accessPointName; }
String WiFiSetupServer::factoryPassword() const { return _accessPointPassword; }
IPAddress WiFiSetupServer::accessPointIP() const { return WiFi.softAPIP(); }
void WiFiSetupServer::handleRoot() { _server.send_P(200, "text/html", SETUP_PAGE); }

void WiFiSetupServer::handleSave() {
    if (!_server.hasArg("ssid") || _server.arg("ssid").isEmpty()) {
        _server.send(400, "application/json", "{\"message\":\"WiFi network name is required\"}");
        return;
    }
    String ssid = _server.arg("ssid"), password = _server.arg("password");
    if (ssid.length() > 32 || password.length() > 63) {
        _server.send(400, "application/json", "{\"message\":\"Invalid WiFi details\"}");
        return;
    }
    WiFi.begin(ssid.c_str(), password.c_str());
    _connectPending = true;
    _connectStartedAt = millis();
    _server.send(202, "application/json", "{\"message\":\"Credentials saved; connecting...\"}");
}

void WiFiSetupServer::handleStatus() { _server.send(200, "application/json", statusJson()); }

String WiFiSetupServer::statusJson() const {
    bool connected = WiFi.status() == WL_CONNECTED;
    return "{\"serial\":\"" + jsonEscape(_serialNumber) + "\",\"ap\":\"" + jsonEscape(_accessPointName) +
           "\",\"connected\":" + (connected ? "true" : "false") + ",\"connecting\":" + (_connectPending ? "true" : "false") +
           ",\"ssid\":\"" + jsonEscape(connected ? WiFi.SSID() : "") + "\",\"ip\":\"" + (connected ? WiFi.localIP().toString() : "") + "\"}";
}

String WiFiSetupServer::jsonEscape(const String &value) {
    String result;
    for (size_t i = 0; i < value.length(); ++i) {
        char c = value.charAt(i);
        if (c == '\\' || c == '"') result += '\\';
        if (static_cast<uint8_t>(c) >= 0x20) result += c;
    }
    return result;
}

