#ifndef EEPROM_STORAGE_H
#define EEPROM_STORAGE_H

#include <Arduino.h>
#include <stdint.h>
#include <stdbool.h>
#include "at24c32N.h"

class EEPROMStorage
{
public:

    EEPROMStorage();

    bool begin(uint8_t sdaPin,
               uint8_t sclPin,
               uint8_t deviceAddress = AT24C32_I2C_ADDR,
               uint32_t freq = 100000);

    bool readBytes(uint16_t addr,
                   uint8_t* data,
                   uint16_t len);

    bool writeBytes(uint16_t addr,
                    const uint8_t* data,
                    uint16_t len);

    bool writeUint16(uint16_t addr,
                     uint16_t value);

    bool readUint16(uint16_t addr,
                    uint16_t &value);

    bool writeUint32(uint16_t addr,
                     uint32_t value);

    bool readUint32(uint16_t addr,
                    uint32_t &value);

    bool writeFloat(uint16_t addr,
                    float value);

    bool readFloat(uint16_t addr,
                   float &value);
};

#endif
