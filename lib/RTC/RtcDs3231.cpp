#include "RtcDs3231.h"

RtcDs3231::RtcDs3231() : _initialized(false) {}

bool RtcDs3231::begin() {
    Wire.begin();
    _initialized = rtc.begin();
    return _initialized;
}

bool RtcDs3231::isInitialized() const {
    return _initialized;
}

bool RtcDs3231::isRunning() {
    return _initialized && !rtc.lostPower();
}

DateTime RtcDs3231::now() {
    if (!_initialized) return DateTime((uint32_t)0);
    return rtc.now();
}

String RtcDs3231::readTimeString() {
    if (!_initialized) return String();
    DateTime t = rtc.now();
    char buf[16];
    snprintf(buf, sizeof(buf), "%02u:%02u:%02u", t.hour(), t.minute(), t.second());
    return String(buf);
}

String RtcDs3231::readDateTimeString() {
    if (!_initialized) return String();
    DateTime t = rtc.now();
    char buf[32];
    snprintf(buf, sizeof(buf), "%04u-%02u-%02u %02u:%02u:%02u",
             t.year(), t.month(), t.day(), t.hour(), t.minute(), t.second());
    return String(buf);
}

String RtcDs3231::readDT_As_ddmmyyyyhhmmss() {
    if (!_initialized) return String();
    DateTime t = rtc.now();
    char buf[32];
    snprintf(buf, sizeof(buf), "%02u-%02u-%04u %02u:%02u:%02u",
             t.day(), t.month(), t.year(),
             t.hour(), t.minute(), t.second());
    return String(buf);
}

bool RtcDs3231::setDateTimeFromString(const String &dateTimeStr) {
    if (!_initialized) return false;

    uint16_t year;
    uint8_t month, day, hour, minute, second;
    int parsed = sscanf(dateTimeStr.c_str(), "%2hhu-%2hhu-%4hu %2hhu:%2hhu:%2hhu",
                        &day, &month, &year, &hour, &minute, &second);
    if (parsed != 6) return false;

    rtc.adjust(DateTime(year, month, day, hour, minute, second));
    return true;
}

bool RtcDs3231::setDateTimeFromCompactString(const String &str) {
    if (!_initialized || str.length() != 14) return false;

    uint8_t day = str.substring(0, 2).toInt();
    uint8_t month = str.substring(2, 4).toInt();
    uint16_t year = str.substring(4, 8).toInt();
    uint8_t hour = str.substring(8, 10).toInt();
    uint8_t minute = str.substring(10, 12).toInt();
    uint8_t second = str.substring(12, 14).toInt();

    rtc.adjust(DateTime(year, month, day, hour, minute, second));
    return true;
}

bool RtcDs3231::setDateTime(const DateTime &dt) {
    if (!_initialized) return false;
    rtc.adjust(dt);
    return true;
}

bool RtcDs3231::setDateTime(uint16_t year, uint8_t month, uint8_t day,
                            uint8_t hour, uint8_t minute, uint8_t second) {
    if (!_initialized) return false;
    rtc.adjust(DateTime(year, month, day, hour, minute, second));
    return true;
}

bool RtcDs3231::setTime(uint8_t hour, uint8_t minute, uint8_t second) {
    if (!_initialized) return false;

    DateTime current = rtc.now();
    rtc.adjust(DateTime(current.year(), current.month(), current.day(),
                        hour, minute, second));
    return true;
}

bool RtcDs3231::updateIfStopped() {
    if (!_initialized) return false;
    if (rtc.lostPower()) {
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
    return true;
}

