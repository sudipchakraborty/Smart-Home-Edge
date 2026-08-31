#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H
#include <Arduino.h>

class OTAManager {
public:
    using UpdateReceivedCallback = void (*)();

    OTAManager();
    void begin(const String &currentVersion);
    void setURLs(const String &versionURL, const String &firmwareURL = "");
    void setRootCA(const char *cert);
    void setDebugStream(Stream &out);
    void setAutoReboot(bool enable);
    void setCheckInterval(uint32_t intervalMs);
    void setUpdateReceivedCallback(UpdateReceivedCallback callback);
    void handle();
    bool checkAndUpdate();
    String getCurrentVersion() const;
    String getAvailableVersion() const;
private:
    String currentVersion;
    String availableVersion;
    String versionURL;
    String firmwareURL;
    const char *rootCA;
    Stream *debug;
    bool autoReboot;
    uint32_t checkIntervalMs;
    uint32_t lastCheckAt;
    bool firstCheckPending;
    uint8_t lastProgressPercent;
    UpdateReceivedCallback updateReceivedCallback;
    bool fetchVersion();
    bool performOTA();
    void log(const String &message);
};
#endif
