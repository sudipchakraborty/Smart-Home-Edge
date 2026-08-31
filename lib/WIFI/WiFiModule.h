#ifndef WIFI_MODULE_H
#define WIFI_MODULE_H

#include <Arduino.h>
#include <WiFi.h>
#include "DEBUG.h"

class WiFiModule {
public:
    WiFiModule();

    bool open();
    void close();

    bool connect(const char *ssid, const char *password,
                 uint32_t timeoutMs = 15000);
    bool connectSaved(uint32_t timeoutMs = 15000);
    bool begin(const char *ssid, const char *password, debug &dbg,
               uint32_t timeoutMs = 15000);
    void disconnect(bool eraseCredentials = false);
    bool isConnected() const;
    IPAddress localIP() const;
    IPAddress gatewayIP() const;
    IPAddress dnsIP(uint8_t index = 0) const;

    bool openConnection(const char *host, uint16_t port,
                        uint32_t timeoutMs = 5000);
    void closeConnection();
    bool connectionOpen();

    size_t send(const uint8_t *data, size_t length);
    size_t send(const String &data);
    int available();
    int receive(uint8_t *buffer, size_t bufferSize);

private:
    WiFiClient _client;
    bool _radioOpen;
};

#endif // WIFI_MODULE_H

