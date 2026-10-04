#ifndef WDT_MANAGER_H
#define WDT_MANAGER_H

#include <Arduino.h>

class WDTManager {
public:
    bool begin(bool enabled, uint32_t timeoutSeconds, bool triggerPanic);
    void feed();
    bool enabled() const;
    uint32_t timeoutSeconds() const;

private:
    bool _enabled = false;
    bool _started = false;
    uint32_t _timeoutSeconds = 0;
};

#endif // WDT_MANAGER_H
