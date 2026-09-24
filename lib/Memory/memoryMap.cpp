#include "memoryMap.h"
#include <cstddef>

namespace
{
void loadASCII(uint16_t firstRegister, uint16_t lastRegister, const char *value)
{
    const size_t capacity = lastRegister - firstRegister + 1;
    for (size_t i = 0; i < capacity; ++i)
        modbusMemory[firstRegister + i] = value[i] ? static_cast<uint8_t>(value[i]) : ' ';
}
}


void ModbusMemory_LoadTestPattern()
{
    for (uint16_t i = 0; i < MODBUS_MEM_SIZE; i++)
    {
        uint8_t val = i % 100;   // keep it simple (0–99)

        // Pattern: 0x0011, 0x1122, 0x2233 ...
        modbusMemory[i] = (val << 8) | ((val + 1) % 100);
    }
}

/* -------------------------------------------------
 * Global Modbus Data (DEFINITION)
 * This must exist in exactly ONE .cpp file
 * ------------------------------------------------- */
// ModbusData_t modbusData = {0};

uint16_t modbusMemory[MODBUS_MEM_SIZE] = {0};

void ModbusMemory_LoadProductIdentity()
{
    // Load the configured strings once during startup. Each register contains
    // one ASCII character, allowing a standard function 03 read to fetch them.
    loadASCII(MODBUS_PRODUCT_ID_FIRST, MODBUS_PRODUCT_ID_LAST, "LPTM-V1.0");
    loadASCII(MODBUS_SERIAL_NUMBER_FIRST, MODBUS_SERIAL_NUMBER_LAST, "IN-001-2609=0001");
}

bool ModbusMemory_IsReadOnly(uint16_t address)
{
    return (address >= MODBUS_PRODUCT_ID_FIRST && address <= MODBUS_PRODUCT_ID_LAST) ||
           (address >= MODBUS_SERIAL_NUMBER_FIRST && address <= MODBUS_SERIAL_NUMBER_LAST);
}

/* -------------------------------------------------
 * Read Holding Register
 * Address range: 40000+
 * ------------------------------------------------- */
// uint16_t modbusMemoryRead(uint16_t address)
// {
//     switch (address)
//     {
//         case 0: return modbusData.OUTPUTS;
//         case 1: return modbusData.START_HH_MM;
//         case 2: return modbusData.END_HH_MM;
//         default:
//             return 0;
//     }
// }

/* -------------------------------------------------
 * Write Holding Register
 * ------------------------------------------------- */
// void modbusMemoryWrite(uint16_t address, uint16_t value)
// {
//     switch (address)
//     {
//         case 0: modbusData.OUTPUTS   = value; break;
//         case 1: modbusData.START_HH_MM   = value; break;
//         case 2: modbusData.END_HH_MM      = value; break;
//         default:
//             break;
//     }
// }
// ///////////////////////////////////////////////////////////////
