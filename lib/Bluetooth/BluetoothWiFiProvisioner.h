#ifndef BLUETOOTH_WIFI_PROVISIONER_H
#define BLUETOOTH_WIFI_PROVISIONER_H

#include <Arduino.h>
#include <WiFi.h>

class BluetoothWiFiProvisioner {
public:
    BluetoothWiFiProvisioner();

    // Starts a secure BLE setup interface. The phone supplies both SSID and password.
    void begin(const char *deviceName, const char *proofOfPossession,
               bool eraseOldCredentials = false);
    bool isActive() const;
    bool isConnected() const;

private:
    static BluetoothWiFiProvisioner *_instance;
    static void handleSystemEvent(arduino_event_t *event);

    bool _active;
};

#endif

