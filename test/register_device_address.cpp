#include "ModbusDeviceAddress.h"
#include <string.h>
// Replace Arduino-only parser declarations for this host response-builder test.
#define MODBUS_ASCII_H
struct ModbusASCIIFrame {
    bool valid;
    uint8_t slaveId;
    uint8_t function;
    uint16_t address;
    uint16_t quantity;
    uint16_t value;
    uint8_t rawData[64];
    uint8_t rawLen;
};
#include "../lib/Modbus/modbusResponseBuilder.cpp"
uint16_t modbusMemory[MODBUS_MEM_SIZE] = {};
bool ModbusMemory_IsReadOnly(uint16_t address) {
    return address >= MODBUS_PRODUCT_ID_FIRST && address <= MODBUS_SERIAL_NUMBER_LAST;
}
int main() {
    uint16_t memory[200] = {};
    for (unsigned id = 0; id <= 999; ++id) {
        memory[29] = '0' + id / 100;
        memory[30] = '0' + id / 10 % 10;
        memory[31] = '0' + id % 10;
        unsigned expected = id >= 1 && id <= 247 ? id : 0;
        if (ModbusDeviceAddress::read(memory) != expected) return 1;
    }
    memory[29] = '0'; memory[30] = '0'; memory[31] = '1';
    if (!ModbusDeviceAddress::validWrite(memory, 31, '2')) return 2;
    if (ModbusDeviceAddress::read(memory) != 1) return 3; // validation cannot mutate
    memory[31] = '2'; // mimic successful function 06 write
    if (ModbusDeviceAddress::read(memory) != 2) return 4; // next packet uses new address
    if (ModbusDeviceAddress::validWrite(memory, 31, '0')) return 5;
    if (ModbusDeviceAddress::validWrite(memory, 29, '3')) return 6;
    if (ModbusDeviceAddress::validWrite(memory, 30, 'A')) return 7;
    if (ModbusDeviceAddress::validWrite(memory, 31, 0x132)) return 8;
    unsigned char eeprom[] = {'2', '4', '7'};
    if (ModbusDeviceAddress::decode(eeprom) != 247) return 9;
    eeprom[2] = '8';
    if (ModbusDeviceAddress::decode(eeprom) != 0) return 10;
    modbusMemory[29] = '0'; modbusMemory[30] = '0'; modbusMemory[31] = '1';
    ModbusASCIIFrame frame = {};
    frame.valid = true; frame.slaveId = 1; frame.function = 6;
    frame.address = 31; frame.value = '2';
    char response[520] = {};
    if (!Modbus_BuildResponse(&frame, response, sizeof(response))) return 11;
    if (strncmp(response, ":0106001F0032", 13) != 0) return 12; // echo OLD unit ID
    if (ModbusDeviceAddress::read(modbusMemory) != 2) return 13;
    frame.slaveId = 2; frame.value = '0';
    Modbus_BuildResponse(&frame, response, sizeof(response));
    if (strncmp(response, ":028603", 7) != 0) return 14; // illegal value
    if (ModbusDeviceAddress::read(modbusMemory) != 2) return 15;
    frame.function = 3; frame.address = 29; frame.quantity = 3;
    Modbus_BuildResponse(&frame, response, sizeof(response));
    if (strncmp(response, ":020306003000300032", 19) != 0) return 16; // read 002
    return 0;
}
