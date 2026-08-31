#include <Arduino.h>
#include "Utils.h"

namespace Utils
{
    
//________________________________________________________________________________________________
uint16_t getUint16FromBuffer(uint8_t *buffer, uint16_t index)
{
    uint16_t value = ((uint16_t)buffer[index + 1] << 8) | buffer[index];
    return value;
}
//________________________________________________________________________________________________ 
void Convert_Byte_To_Int(const uint8_t *src,
                         uint16_t src_pos,
                         uint16_t *dst,
                         uint16_t dst_pos,
                         uint16_t count)
{
    for (uint16_t i = 0; i < count; i++)
    {
        uint16_t low  = src[src_pos + (i * 2)];
        uint16_t high = src[src_pos + (i * 2) + 1];

        dst[dst_pos + i] = (high << 8) | low;
    }
}
//__________________________________________________________________________________________
void Convert_Int_To_Byte(const uint16_t *src,
                         uint16_t src_pos,
                         uint8_t *dst,
                         uint16_t dst_pos,
                         uint16_t count)
{
    if (!src || !dst) return;

    for (uint16_t i = 0; i < count; i++)
    {
        uint16_t value = src[src_pos + i];

        uint16_t index = dst_pos + (i << 1);   // i * 2

        dst[index]     = (uint8_t)(value & 0xFF);        // Low byte
        dst[index + 1] = (uint8_t)((value >> 8) & 0xFF); // High byte
    }
}
//__________________________________________________________________________________________
uint32_t timeStringToSeconds(const char *timeStr)
{
    if (!timeStr) return 0;

    uint8_t hh = (timeStr[0] - '0') * 10 + (timeStr[1] - '0');
    uint8_t mm = (timeStr[3] - '0') * 10 + (timeStr[4] - '0');
    uint8_t ss = (timeStr[6] - '0') * 10 + (timeStr[7] - '0');

    return (hh * 3600UL) + (mm * 60UL) + ss;
}
//__________________________________________________________________________________________
uint32_t Hex_HHMM_ToSeconds(uint16_t regValue)
{
    uint8_t hour   = (regValue >> 8) & 0xFF;   // upper byte
    uint8_t minute = regValue & 0xFF;          // lower byte

    // Optional safety validation
    if (hour > 23 || minute > 59)
        return 0;  // invalid time

    return (hour * 3600UL) + (minute * 60UL);
}
//__________________________________________________________________________________________
uint32_t Get_StrHHMM_ToSeconds(const char *timeStr)
{
    if (!timeStr) return 0;

    // Basic format validation: "HH:MM"
    if (timeStr[2] != ':')
        return 0;

    uint8_t hour   = (timeStr[0] - '0') * 10 + (timeStr[1] - '0');
    uint8_t minute = (timeStr[3] - '0') * 10 + (timeStr[4] - '0');

    if (hour > 23 || minute > 59)
        return 0;

    return (hour * 3600UL) + (minute * 60UL);
}

//__________________________________________________________________________________________
uint32_t Get_StrHHMMSS_ToSeconds(const char *timeStr)
{
    if (!timeStr) return 0;

    // Validate format: "HH:MM:SS"
    if (timeStr[2] != ':' || timeStr[5] != ':')
        return 0;

    uint8_t hour   = (timeStr[0] - '0') * 10 + (timeStr[1] - '0');
    uint8_t minute = (timeStr[3] - '0') * 10 + (timeStr[4] - '0');
    uint8_t second = (timeStr[6] - '0') * 10 + (timeStr[7] - '0');

    // Range validation
    if (hour > 23 || minute > 59 || second > 59)
        return 0;

    return (hour * 3600UL) +
           (minute * 60UL) +
           second;
}
//__________________________________________________________________________________________
uint32_t DT_String_To_Seconds_From_TimePart(const char *dt)
{
    if (!dt) return 0;

    // Move pointer to time part "hh:mm:ss"
    dt += 11;   // skip "dd-mm-yyyy "

    uint8_t hh = (dt[0] - '0') * 10 + (dt[1] - '0');
    uint8_t mm = (dt[3] - '0') * 10 + (dt[4] - '0');
    uint8_t ss = (dt[6] - '0') * 10 + (dt[7] - '0');

    return (uint32_t)hh * 3600UL + (uint32_t)mm * 60UL + ss;
}
//__________________________________________________________________________________________
void storeUint32ToModbus(uint16_t *modbusMemory,uint16_t startAddr,uint32_t value)
{
    // Lower 16-bit
    modbusMemory[startAddr] = (uint16_t)(value & 0xFFFF);

    // Upper 16-bit
    modbusMemory[startAddr + 1] = (uint16_t)((value >> 16) & 0xFFFF);
}
//__________________________________________________________________________________________
uint32_t readUint32FromModbus(uint16_t *modbusMemory,uint16_t startAddr)
{
    uint32_t low  = modbusMemory[startAddr];
    uint32_t high = modbusMemory[startAddr + 1];

    return (high << 16) | low;
}
//__________________________________________________________________________________________
String secondsToHHMMSS(uint32_t totalSeconds)
{
    if (totalSeconds > 86399UL)
    {
        return String("INVALID(") + String(totalSeconds) + ")";
    }

    const uint32_t hh = totalSeconds / 3600UL;
    const uint32_t mm = (totalSeconds % 3600UL) / 60UL;
    const uint32_t ss = totalSeconds % 60UL;

    char buf[12];

    sprintf(buf,
            "%02lu:%02lu:%02lu",
            static_cast<unsigned long>(hh),
            static_cast<unsigned long>(mm),
            static_cast<unsigned long>(ss));

    return String(buf);
}


///////////////////////////////////////////////////////////////////////////////////////////
}
