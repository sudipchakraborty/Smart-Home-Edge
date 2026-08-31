#include "OTAManager.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <Update.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

OTAManager::OTAManager()
    : rootCA(nullptr), debug(nullptr), autoReboot(true),
      checkIntervalMs(21600000UL), lastCheckAt(0), firstCheckPending(true),
      lastProgressPercent(255), updateReceivedCallback(nullptr) {}

void OTAManager::begin(const String &version) { currentVersion = version; }
void OTAManager::setURLs(const String &manifest, const String &firmware) { versionURL = manifest; firmwareURL = firmware; }
void OTAManager::setRootCA(const char *cert) { rootCA = cert; }
void OTAManager::setDebugStream(Stream &out) { debug = &out; }
void OTAManager::setAutoReboot(bool enable) { autoReboot = enable; }
void OTAManager::setCheckInterval(uint32_t value) { checkIntervalMs = value < 60000UL ? 60000UL : value; }
void OTAManager::setUpdateReceivedCallback(UpdateReceivedCallback callback) { updateReceivedCallback = callback; }
String OTAManager::getCurrentVersion() const { return currentVersion; }
String OTAManager::getAvailableVersion() const { return availableVersion; }
void OTAManager::log(const String &message) { if (debug) debug->println("[OTA] " + message); }

void OTAManager::handle() {
    if (WiFi.status() != WL_CONNECTED || versionURL.isEmpty()) return;
    uint32_t now = millis();
    if (firstCheckPending) {
        if (now < 30000UL) return;
        firstCheckPending = false;
    } else if (now - lastCheckAt < checkIntervalMs) return;
    lastCheckAt = now;
    checkAndUpdate();
}
bool OTAManager::checkAndUpdate() {
    if (WiFi.status() != WL_CONNECTED || versionURL.isEmpty()) return false;
    if (!fetchVersion()) return false;
    if (availableVersion.isEmpty() || availableVersion == currentVersion) {
        log("Firmware is current: " + currentVersion);
        return false;
    }
    log("Update received: version " + availableVersion);
    log("Current version: " + currentVersion);
    if (updateReceivedCallback) updateReceivedCallback();
    return performOTA();
}

bool OTAManager::fetchVersion() {
    WiFiClientSecure client;
    if (rootCA) client.setCACert(rootCA); else client.setInsecure();
    HTTPClient http;
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setTimeout(15000);
    if (!http.begin(client, versionURL)) { log("Manifest connection failed"); return false; }
    int code = http.GET();
    if (code != HTTP_CODE_OK) { log("Manifest HTTP error: " + String(code)); http.end(); return false; }
    JsonDocument document;
    DeserializationError error = deserializeJson(document, http.getString());
    if (error || !document["version"].is<const char *>()) {
        log("Invalid OTA manifest"); http.end(); return false;
    }
    availableVersion = document["version"].as<String>();
    if (document["url"].is<const char *>()) firmwareURL = document["url"].as<String>();
    http.end();
    if (firmwareURL.isEmpty()) { log("Firmware URL missing from manifest"); return false; }
    return true;
}

bool OTAManager::performOTA() {
    WiFiClientSecure client;
    if (rootCA) client.setCACert(rootCA); else client.setInsecure();
    HTTPClient http;
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setTimeout(30000);
    if (!http.begin(client, firmwareURL)) { log("Firmware connection failed"); return false; }
    int code = http.GET();
    if (code != HTTP_CODE_OK) { log("Firmware HTTP error: " + String(code)); http.end(); return false; }
    int contentLength = http.getSize();
    size_t updateSize = contentLength > 0 ? static_cast<size_t>(contentLength) : UPDATE_SIZE_UNKNOWN;
    if (!Update.begin(updateSize)) { log("Inactive OTA partition is too small"); http.end(); return false; }
    lastProgressPercent = 255;
    Update.onProgress([this](size_t progress, size_t total) {
        if (total == 0 || total == UPDATE_SIZE_UNKNOWN) return;
        uint8_t percent = static_cast<uint8_t>((progress * 100ULL) / total);
        if (percent != lastProgressPercent) {
            lastProgressPercent = percent;
            log("Update progress: " + String(percent) + "%");
        }
    });
    log("Update started: downloading firmware");
    size_t written = Update.writeStream(*http.getStreamPtr());
    if (contentLength > 0 && written != static_cast<size_t>(contentLength)) {
        log("Incomplete firmware download"); Update.abort(); http.end(); return false;
    }
    if (!Update.end() || !Update.isFinished()) {
        log("Firmware validation failed, error: " + String(Update.getError())); http.end(); return false;
    }
    http.end();
    if (lastProgressPercent != 100) log("Update progress: 100%");
    log("Update completed successfully");
    if (autoReboot) {
        log("System will restart in 3 seconds");
        for (int seconds = 3; seconds > 0; --seconds) {
            log("Restarting in " + String(seconds) + "...");
            delay(1000);
        }
        log("Restarting now with updated firmware");
        if (debug) debug->flush();
        ESP.restart();
    }
    return true;
}
