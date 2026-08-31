#ifndef WIFI_SETUP_SERVER_H
#define WIFI_SETUP_SERVER_H
#include <Arduino.h>
#include <DNSServer.h>
#include <WebServer.h>

class WiFiSetupServer {
public:
    explicit WiFiSetupServer(uint16_t port = 80);
    bool begin(const String &hotspotName, const String &hotspotPassword,
               const String &serialNumber);
    void handleClient();
    String accessPointName() const;
    String factoryPassword() const;
    IPAddress accessPointIP() const;
private:
    WebServer _server;
    DNSServer _dns;
    String _serialNumber;
    String _accessPointName;
    String _accessPointPassword;
    bool _connectPending;
    uint32_t _connectStartedAt;
    void handleRoot();
    void handleSave();
    void handleStatus();
    String statusJson() const;
    static String jsonEscape(const String &value);
};
#endif

