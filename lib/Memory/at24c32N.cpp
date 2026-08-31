#include "at24c32N.h"
#include <Arduino.h>
#include <Wire.h>

/* -------- Internal Helpers -------- */

static inline uint16_t regToByte(uint16_t reg)
{
    return reg * 2;
}

bool i2cWrite(uint16_t memAddr, const uint8_t* data, size_t len)
{
    Wire.beginTransmission(AT24C32_I2C_ADDR);
    Wire.write((memAddr >> 8) & 0xFF);
    Wire.write(memAddr & 0xFF);

    for (size_t i = 0; i < len; i++)
        Wire.write(data[i]);

    return (Wire.endTransmission() == 0);
}

bool i2cRead(uint16_t memAddr, uint8_t* data, size_t len)
{
    Wire.beginTransmission(AT24C32_I2C_ADDR);
    Wire.write((memAddr >> 8) & 0xFF);
    Wire.write(memAddr & 0xFF);
    if (Wire.endTransmission(false) != 0)
        return false;

    Wire.requestFrom(static_cast<uint8_t>(AT24C32_I2C_ADDR), static_cast<uint8_t>(len));
    for (size_t i = 0; i < len; i++)
    {
        if (!Wire.available())
            return false;
        data[i] = Wire.read();
    }
    return true;
}

/* -------- Public API -------- */

void AT24C32_Init(uint8_t sclPin, uint8_t sdaPin, uint32_t freq)
{
    Wire.begin(sdaPin, sclPin, freq);
}

bool AT24C32_Probe()
{
    Wire.beginTransmission(AT24C32_I2C_ADDR);
    return Wire.endTransmission() == 0;
}

bool AT24C32_ReadRegister(uint16_t regAddr, uint16_t* value)
{
    if (!value) return false;

    uint16_t memAddr = regToByte(regAddr);
    if (memAddr + 1 >= AT24C32_TOTAL_BYTES)
        return false;

    uint8_t buf[2];
    if (!i2cRead(memAddr, buf, 2))
        return false;

    *value = (buf[0] << 8) | buf[1];
    return true;
}

bool AT24C32_ReadRegisters(uint16_t startReg,
                           uint16_t count,
                           uint16_t* buffer)
{
    if (!buffer || count == 0)
        return false;

    uint16_t memAddr = regToByte(startReg);
    uint16_t bytes   = count * 2;

    if (memAddr + bytes > AT24C32_TOTAL_BYTES)
        return false;

    uint8_t temp[64];
    if (bytes > sizeof(temp))
        return false;

    if (!i2cRead(memAddr, temp, bytes))
        return false;

    for (uint16_t i = 0; i < count; i++)
    {
        buffer[i] = (temp[i * 2] << 8) | temp[i * 2 + 1];
    }
    return true;
}

bool AT24C32_WriteRegister(uint16_t regAddr, uint16_t value)
{
    uint16_t memAddr = regToByte(regAddr);
    if (memAddr + 1 >= AT24C32_TOTAL_BYTES)
        return false;

    uint8_t buf[2];
    buf[0] = value >> 8;
    buf[1] = value & 0xFF;

    if (!i2cWrite(memAddr, buf, 2))
        return false;

    delay(5); // EEPROM write cycle
    return true;
}

bool AT24C32_WriteRegisters(uint16_t startReg,
                            uint16_t count,
                            const uint16_t* buffer)
{
    if (!buffer || count == 0)
        return false;

    uint16_t memAddr = regToByte(startReg);
    uint16_t bytes   = count * 2;

    if (memAddr + bytes > AT24C32_TOTAL_BYTES)
        return false;

    uint8_t temp[64];
    if (bytes > sizeof(temp))
        return false;

    for (uint16_t i = 0; i < count; i++)
    {
        temp[i * 2]     = buffer[i] >> 8;
        temp[i * 2 + 1] = buffer[i] & 0xFF;
    }

    uint16_t offset = 0;
    while (offset < bytes)
    {
        uint16_t pageOffset = (memAddr + offset) % AT24C32_PAGE_SIZE;
        uint16_t space      = AT24C32_PAGE_SIZE - pageOffset;
        uint16_t chunk      = (bytes - offset > space) ? space : (bytes - offset);

        if (!i2cWrite(memAddr + offset, &temp[offset], chunk))
            return false;

        delay(5);
        offset += chunk;
    }
    return true;
}
//////////////////////////////////////////////////////////////////////////////////
int32_t AT24C32_Read3BytesAsInt(uint16_t startAddr)
{
    uint8_t buf[3];

    // Boundary check (AT24C32 = 4096 bytes)
    if (startAddr + 2 >= AT24C32_TOTAL_BYTES)
        return -1;   // error indicator

    if (!i2cRead(startAddr, buf, 3))
        return -1;   // read failure

    // Combine 3 bytes (BIG-ENDIAN)
    int32_t value =
        ((int32_t)buf[0] << 16) |
        ((int32_t)buf[1] << 8)  |
         (int32_t)buf[2];

    return value;
}
/////////////////////////////////////////////////////////////////////////////////
