#include "WiFiModule.h"

WiFiModule::WiFiModule() : _radioOpen(false) {}

bool WiFiModule::open() {
    WiFi.mode(WIFI_STA);
    _radioOpen = true;
    return true;
}

void WiFiModule::close() {
    closeConnection();
    WiFi.disconnect(true, false);
    WiFi.mode(WIFI_OFF);
    _radioOpen = false;
}

bool WiFiModule::connect(const char *ssid, const char *password,
                         uint32_t timeoutMs) {
    if (ssid == nullptr || ssid[0] == '\0') return false;
    if (!_radioOpen) open();

    WiFi.begin(ssid, password);
    uint32_t startedAt = millis();
    while (WiFi.status() != WL_CONNECTED &&
           (millis() - startedAt) < timeoutMs) {
        delay(100);
    }
    return WiFi.status() == WL_CONNECTED;
}

bool WiFiModule::connectSaved(uint32_t timeoutMs) {
    if (!_radioOpen) open();
    WiFi.begin();
    uint32_t startedAt = millis();
    while (WiFi.status() != WL_CONNECTED &&
           (millis() - startedAt) < timeoutMs) {
        delay(100);
    }
    return WiFi.status() == WL_CONNECTED;
}

bool WiFiModule::begin(const char *ssid, const char *password, debug &dbg,
                       uint32_t timeoutMs) {
    if (!connect(ssid, password, timeoutMs)) {
        dbg.println("WiFi Connection ERROR - Using Existing RTC Time");
        return false;
    }

    dbg.println("WiFi Connected");
    dbg.println("WiFi IP: ", localIP().toString());
    dbg.println("Gateway: ", gatewayIP().toString());
    dbg.println("DNS: ", dnsIP().toString());
    return true;
}

void WiFiModule::disconnect(bool eraseCredentials) {
    closeConnection();
    WiFi.disconnect(false, eraseCredentials);
}

bool WiFiModule::isConnected() const {
    return WiFi.status() == WL_CONNECTED;
}

IPAddress WiFiModule::localIP() const {
    return WiFi.localIP();
}

IPAddress WiFiModule::gatewayIP() const {
    return WiFi.gatewayIP();
}

IPAddress WiFiModule::dnsIP(uint8_t index) const {
    return WiFi.dnsIP(index);
}

bool WiFiModule::openConnection(const char *host, uint16_t port,
                                uint32_t timeoutMs) {
    if (!isConnected() || host == nullptr || host[0] == '\0') return false;
    closeConnection();
    return _client.connect(host, port, timeoutMs);
}

void WiFiModule::closeConnection() {
    _client.stop();
}

bool WiFiModule::connectionOpen() {
    return _client.connected();
}

size_t WiFiModule::send(const uint8_t *data, size_t length) {
    if (!connectionOpen() || data == nullptr) return 0;
    return _client.write(data, length);
}

size_t WiFiModule::send(const String &data) {
    return send(reinterpret_cast<const uint8_t *>(data.c_str()), data.length());
}

int WiFiModule::available() {
    return _client.available();
}

int WiFiModule::receive(uint8_t *buffer, size_t bufferSize) {
    if (buffer == nullptr || bufferSize == 0) return 0;
    return _client.read(buffer, bufferSize);
}

