#ifndef I2C_SCANNER_H
#define I2C_SCANNER_H

#include <Arduino.h>
#include <Wire.h>

class I2CScanner {
public:
    // Scans standard usable 7-bit I2C addresses on an initialized bus.
    // Returns the number of responding devices.
    static uint8_t scan(Stream &output, TwoWire &bus = Wire);

    static bool devicePresent(uint8_t address, TwoWire &bus = Wire);
};

#endif

