#include "ClockInternet.h"

#include <WiFi.h>
#include <time.h>

ClockInternet::ClockInternet()
    : _initialized(false), _lastError(ClockInternetError::NONE) {}

bool ClockInternet::begin(long gmtOffsetSeconds,
                          int daylightOffsetSeconds,
                          const char *primaryServer,
                          const char *secondaryServer) {
    if (WiFi.status() != WL_CONNECTED) {
        _initialized = false;
        _lastError = ClockInternetError::WIFI_DISCONNECTED;
        return false;
    }

    bool primaryResolved = WiFi.hostByName(primaryServer, _primaryServerIP);
    bool secondaryResolved = WiFi.hostByName(secondaryServer, _secondaryServerIP);
    if (!primaryResolved && !secondaryResolved) {
        _initialized = false;
        _lastError = ClockInternetError::DNS_ERROR;
        return false;
    }

    configTime(gmtOffsetSeconds, daylightOffsetSeconds,
               primaryServer, secondaryServer);
    _initialized = true;
    _lastError = ClockInternetError::NONE;
    return true;
}

bool ClockInternet::readDateTime(DateTime &dateTime, uint32_t timeoutMs) {
    if (WiFi.status() != WL_CONNECTED) {
        _lastError = ClockInternetError::WIFI_DISCONNECTED;
        return false;
    }
    if (!_initialized) {
        _lastError = ClockInternetError::NOT_INITIALIZED;
        return false;
    }

    struct tm timeInfo = {};
    uint32_t startedAt = millis();
    bool timeReceived = false;
    while ((millis() - startedAt) < timeoutMs) {
        if (getLocalTime(&timeInfo, 500)) {
            timeReceived = true;
            break;
        }
        delay(50);
    }

    if (!timeReceived) {
        _lastError = ClockInternetError::NTP_TIMEOUT;
        return false;
    }
    if (timeInfo.tm_year + 1900 < 2024) {
        _lastError = ClockInternetError::INVALID_TIME;
        return false;
    }

    dateTime = DateTime(timeInfo.tm_year + 1900,
                        timeInfo.tm_mon + 1,
                        timeInfo.tm_mday,
                        timeInfo.tm_hour,
                        timeInfo.tm_min,
                        timeInfo.tm_sec);
    _lastError = ClockInternetError::NONE;
    return true;
}

bool ClockInternet::isTimeValid() {
    if (!_initialized) return false;
    struct tm timeInfo;
    return getLocalTime(&timeInfo, 100) && (timeInfo.tm_year + 1900 >= 2024);
}

bool ClockInternet::syncRtc(RtcDs3231 &rtc, debug &dbg,
                            long gmtOffsetSeconds,
                            int daylightOffsetSeconds,
                            uint32_t timeoutMs) {
    DateTime internetDateTime;

    if (!begin(gmtOffsetSeconds, daylightOffsetSeconds)) {
        if (_lastError == ClockInternetError::DNS_ERROR) {
            dbg.println("NTP DNS ERROR");
        } else if (_lastError == ClockInternetError::WIFI_DISCONNECTED) {
            dbg.println("NTP START ERROR: WiFi Disconnected");
        } else {
            dbg.println("NTP START ERROR");
        }
        return false;
    }

    dbg.println("NTP Primary IP: ", primaryServerIP().toString());
    dbg.println("NTP Secondary IP: ", secondaryServerIP().toString());
    dbg.println("Waiting For Internet Time...");

    if (!readDateTime(internetDateTime, timeoutMs)) {
        switch (_lastError) {
            case ClockInternetError::WIFI_DISCONNECTED:
                dbg.println("NTP ERROR: WiFi Disconnected");
                break;
            case ClockInternetError::NTP_TIMEOUT:
                dbg.println("NTP TIMEOUT: Check Internet And UDP Port 123");
                break;
            case ClockInternetError::INVALID_TIME:
                dbg.println("NTP ERROR: Invalid Date/Time Received");
                break;
            default:
                dbg.println("NTP READ ERROR");
                break;
        }
        return false;
    }

    if (!rtc.setDateTime(internetDateTime)) {
        dbg.println("RTC WRITE ERROR");
        return false;
    }

    dbg.println("Internet Time: ", rtc.readDT_As_ddmmyyyyhhmmss());
    dbg.println("RTC Updated From Internet");
    return true;
}

ClockInternetError ClockInternet::lastError() const {
    return _lastError;
}

IPAddress ClockInternet::primaryServerIP() const {
    return _primaryServerIP;
}

IPAddress ClockInternet::secondaryServerIP() const {
    return _secondaryServerIP;
}

