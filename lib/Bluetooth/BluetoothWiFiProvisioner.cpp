#include "BluetoothWiFiProvisioner.h"

#include <WiFiProv.h>

BluetoothWiFiProvisioner *BluetoothWiFiProvisioner::_instance = nullptr;

BluetoothWiFiProvisioner::BluetoothWiFiProvisioner() : _active(false) {}

void BluetoothWiFiProvisioner::begin(const char *deviceName,
                                     const char *proofOfPossession,
                                     bool eraseOldCredentials) {
    if (_active) return;
    _instance = this;
    _active = true;
    WiFi.onEvent(handleSystemEvent);

    static uint8_t serviceUuid[16] = {
        0xb4, 0xdf, 0x5a, 0x1c, 0x3f, 0x6b, 0xf4, 0xbf,
        0xea, 0x4a, 0x82, 0x03, 0x04, 0x90, 0x1a, 0x02
    };

    WiFiProv.beginProvision(WIFI_PROV_SCHEME_BLE,
                            WIFI_PROV_SCHEME_HANDLER_FREE_BTDM,
                            WIFI_PROV_SECURITY_1,
                            proofOfPossession,
                            deviceName,
                            nullptr,
                            serviceUuid,
                            eraseOldCredentials);
    WiFiProv.printQR(deviceName, proofOfPossession, "ble");
}

bool BluetoothWiFiProvisioner::isActive() const { return _active; }

bool BluetoothWiFiProvisioner::isConnected() const {
    return WiFi.status() == WL_CONNECTED;
}

void BluetoothWiFiProvisioner::handleSystemEvent(arduino_event_t *event) {
    if (_instance == nullptr) return;

    switch (event->event_id) {
        case ARDUINO_EVENT_PROV_START:
            Serial.println("BLE WiFi setup started");
            Serial.println("Open the Espressif Provisioning app and select this device");
            break;
        case ARDUINO_EVENT_PROV_CRED_RECV:
            // Do not print the received password.
            Serial.print("Received WiFi network: ");
            Serial.println(reinterpret_cast<const char *>(event->event_info.prov_cred_recv.ssid));
            break;
        case ARDUINO_EVENT_PROV_CRED_FAIL:
            Serial.println("WiFi provisioning failed; check network name and password");
            break;
        case ARDUINO_EVENT_PROV_CRED_SUCCESS:
            Serial.println("WiFi credentials saved successfully");
            break;
        case ARDUINO_EVENT_PROV_END:
            _instance->_active = false;
            Serial.println("BLE WiFi setup finished");
            break;
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            Serial.print("WiFi connected. IP: ");
            Serial.println(WiFi.localIP());
            break;
        default:
            break;
    }
}

