#include "EEPROMStorage.h"
#include "at24c32N.h"

EEPROMStorage::EEPROMStorage()
{

}

/* -------------------------------------------------
 * Initialize EEPROM
 * ------------------------------------------------- */
bool EEPROMStorage::begin(uint8_t sdaPin,
                          uint8_t sclPin,
                          uint32_t freq)
{
    AT24C32_Init(sclPin, sdaPin, freq);
    return AT24C32_Probe();
}

/* -------------------------------------------------
 * Read Bytes
 * ------------------------------------------------- */
bool EEPROMStorage::readBytes(uint16_t addr,
                              uint8_t* data,
                              uint16_t len)
{
    if (data == nullptr || len == 0 || addr >= AT24C32_TOTAL_BYTES ||
        static_cast<uint32_t>(addr) + len > AT24C32_TOTAL_BYTES)
        return false;
    return i2cRead(addr, data, len);
}

/* -------------------------------------------------
 * Write Bytes
 * ------------------------------------------------- */
bool EEPROMStorage::writeBytes(uint16_t addr,const uint8_t* data,uint16_t len)
{
    if (data == nullptr || len == 0 || addr >= AT24C32_TOTAL_BYTES ||
        static_cast<uint32_t>(addr) + len > AT24C32_TOTAL_BYTES)
        return false;
    bool ret;
    ret = i2cWrite(addr, (uint8_t*)data, len);
    delay(10);   // EEPROM internal write cycle
    return ret;
}
/* -------------------------------------------------
 * Write uint16_t
 * ------------------------------------------------- */
bool EEPROMStorage::writeUint16(uint16_t addr,
                                uint16_t value)
{
    return writeBytes(addr,
                      (uint8_t*)&value,
                      sizeof(uint16_t));
}

/* -------------------------------------------------
 * Read uint16_t
 * ------------------------------------------------- */
bool EEPROMStorage::readUint16(uint16_t addr,
                               uint16_t &value)
{
    return readBytes(addr,
                     (uint8_t*)&value,
                     sizeof(uint16_t));
}

/* -------------------------------------------------
 * Write uint32_t
 * ------------------------------------------------- */
bool EEPROMStorage::writeUint32(uint16_t addr,
                                uint32_t value)
{
   uint8_t data[4];

    // Store as BIG-ENDIAN
    data[0] = (value >> 24) & 0xFF;
    data[1] = (value >> 16) & 0xFF;
    data[2] = (value >> 8 ) & 0xFF;
    data[3] = (value >> 0 ) & 0xFF;

    return writeBytes(addr, data, 4);
}

/* -------------------------------------------------
 * Read uint32_t
 * ------------------------------------------------- */
bool EEPROMStorage::readUint32(uint16_t addr,
                               uint32_t &value)
{
    uint8_t data[4];

    if (!readBytes(addr, data, 4))
        return false;

    value = 0;

    value |= ((uint32_t)data[0] << 24);
    value |= ((uint32_t)data[1] << 16);
    value |= ((uint32_t)data[2] << 8);
    value |= ((uint32_t)data[3]);

    return true;
}
/* -------------------------------------------------
 * Write Float
 * ------------------------------------------------- */
bool EEPROMStorage::writeFloat(uint16_t addr,
                               float value)
{
    return writeBytes(addr,
                      (uint8_t*)&value,
                      sizeof(float));
}

/* -------------------------------------------------
 * Read Float
 * ------------------------------------------------- */
bool EEPROMStorage::readFloat(uint16_t addr,
                              float &value)
{
    return readBytes(addr,
                     (uint8_t*)&value,
                     sizeof(float));
}
