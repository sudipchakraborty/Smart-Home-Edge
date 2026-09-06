#ifndef UTILS_H
#define UTILS_H

#include <Arduino.h>

namespace Utils
{
    uint16_t getUint16FromBuffer(uint8_t *buffer, uint16_t index);
    void Convert_Byte_To_Int(const uint8_t *src,
                         uint16_t src_pos,
                         uint16_t *dst,
                         uint16_t dst_pos,
                         uint16_t count);

    void Convert_Int_To_Byte(const uint16_t *src,
                         uint16_t src_pos,
                         uint8_t *dst,
                         uint16_t dst_pos,
                         uint16_t count);

uint32_t timeStringToSeconds(const char *timeStr);
uint32_t Hex_HHMM_ToSeconds(uint16_t regValue);
uint32_t Get_StrHHMM_ToSeconds(const char *timeStr);
uint32_t Get_StrHHMMSS_ToSeconds(const char *timeStr);
uint32_t DT_String_To_Seconds_From_TimePart(const char *dt);
bool parseDDMMYYYY_HHMMSS(const char *dateTime,
                          uint16_t &year, uint8_t &month, uint8_t &day,
                          uint8_t &hour, uint8_t &minute, uint8_t &second);
void storeUint32ToModbus(uint16_t *modbusMemory,uint16_t startAddr,uint32_t value);
uint32_t readUint32FromModbus(uint16_t *modbusMemory,uint16_t startAddr);
String secondsToHHMMSS(uint32_t totalSeconds);

}





 




#endif
