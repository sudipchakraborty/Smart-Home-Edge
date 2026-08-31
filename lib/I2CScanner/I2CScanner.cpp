#include "I2CScanner.h"

bool I2CScanner::devicePresent(uint8_t address, TwoWire &bus) {
    if (address < 0x08 || address > 0x77) return false;
    bus.beginTransmission(address);
    return bus.endTransmission() == 0;
}

uint8_t I2CScanner::scan(Stream &output, TwoWire &bus) {
    uint8_t found = 0;
    output.println("I2C scan started");

    for (uint8_t address = 0x08; address <= 0x77; ++address) {
        bus.beginTransmission(address);
        uint8_t result = bus.endTransmission();

        if (result == 0) {
            output.print("I2C device found at 0x");
            if (address < 0x10) output.print('0');
            output.println(address, HEX);
            ++found;
        } else if (result == 4) {
            output.print("I2C bus error at 0x");
            if (address < 0x10) output.print('0');
            output.println(address, HEX);
        }
    }

    output.print("I2C scan completed - devices found: ");
    output.println(found);
    return found;
}

