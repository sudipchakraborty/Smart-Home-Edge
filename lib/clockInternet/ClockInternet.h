#ifndef CLOCK_INTERNET_H
#define CLOCK_INTERNET_H

#include <Arduino.h>
#include <RTClib.h>
#include "DEBUG.h"
#include "RtcDs3231.h"

enum class ClockInternetError {
    NONE,
    WIFI_DISCONNECTED,
    DNS_ERROR,
    NOT_INITIALIZED,
    NTP_TIMEOUT,
    INVALID_TIME
};

class ClockInternet {
public:
    ClockInternet();

    bool begin(long gmtOffsetSeconds = 19800L,
               int daylightOffsetSeconds = 0,
               const char *primaryServer = "pool.ntp.org",
               const char *secondaryServer = "time.nist.gov");

    bool readDateTime(DateTime &dateTime, uint32_t timeoutMs = 10000);
    bool isTimeValid();
    bool syncRtc(RtcDs3231 &rtc, debug &dbg,
                 long gmtOffsetSeconds = 19800L,
                 int daylightOffsetSeconds = 0,
                 uint32_t timeoutMs = 30000);
    ClockInternetError lastError() const;
    IPAddress primaryServerIP() const;
    IPAddress secondaryServerIP() const;

private:
    bool _initialized;
    ClockInternetError _lastError;
    IPAddress _primaryServerIP;
    IPAddress _secondaryServerIP;
};

#endif // CLOCK_INTERNET_H

