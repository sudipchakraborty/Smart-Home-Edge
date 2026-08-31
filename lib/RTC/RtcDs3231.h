#ifndef RTC_DS3231_H
#define RTC_DS3231_H

#include <Arduino.h>
#include <Wire.h>
#include <RTClib.h>

/**
 * @brief Simple DS3231 RTC wrapper using Adafruit RTClib.
 *
 * Provides the same public interface as Rtc1307 so applications can migrate
 * without changing their date/time handling code.
 */
class RtcDs3231 {
public:
    RtcDs3231();

    bool begin();
    bool isInitialized() const;
    bool isRunning();
    DateTime now();

    String readTimeString();
    String readDateTimeString();
    String readDT_As_ddmmyyyyhhmmss();

    bool setDateTimeFromString(const String &dateTimeStr);
    bool setDateTimeFromCompactString(const String &str);
    bool setDateTime(const DateTime &dt);
    bool setDateTime(uint16_t year, uint8_t month, uint8_t day,
                     uint8_t hour, uint8_t minute, uint8_t second);
    bool setTime(uint8_t hour, uint8_t minute, uint8_t second);

    /**
     * @brief Set the RTC to firmware compile time if it reports lost power.
     */
    bool updateIfStopped();

private:
    RTC_DS3231 rtc;
    bool _initialized;
};

#endif // RTC_DS3231_H

