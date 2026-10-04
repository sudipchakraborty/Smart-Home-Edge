#include "WDTManager.h"

#include <esp_task_wdt.h>

bool WDTManager::begin(bool enabled, uint32_t timeoutSeconds, bool triggerPanic)
{
    _enabled = enabled;
    _started = false;
    _timeoutSeconds = timeoutSeconds;

    if (!_enabled) return true;
    if (_timeoutSeconds == 0) return false;

    esp_err_t result = esp_task_wdt_init(_timeoutSeconds, triggerPanic);
    if (result != ESP_OK && result != ESP_ERR_INVALID_STATE) return false;

    result = esp_task_wdt_add(nullptr);
    if (result != ESP_OK) return false;

    _started = true;
    feed();
    return true;
}

void WDTManager::feed()
{
    if (_enabled && _started) esp_task_wdt_reset();
}

bool WDTManager::enabled() const
{
    return _enabled;
}

uint32_t WDTManager::timeoutSeconds() const
{
    return _timeoutSeconds;
}
