#pragma once
#include <stdint.h>

// Shared V1/V2 contract: three ASCII decimal digits in registers 29..31.
namespace ModbusDeviceAddress {
constexpr uint16_t first = 29;
constexpr uint16_t last = 31;

template <typename T> inline uint8_t decode(const T* digits) {
    uint16_t value = 0;
    for (uint8_t i = 0; i < 3; ++i) {
        if (digits[i] < '0' || digits[i] > '9') return 0;
        value = value * 10 + digits[i] - '0';
    }
    return value >= 1 && value <= 247 ? static_cast<uint8_t>(value) : 0;
}
inline uint8_t read(const uint16_t* memory) { return decode(memory + first); }
inline bool validWrite(const uint16_t* memory, uint16_t address, uint16_t value) {
    if (address < first || address > last) return true;
    uint16_t digits[] = {memory[first], memory[first + 1], memory[last]};
    digits[address - first] = value;
    return decode(digits) != 0;
}
}
