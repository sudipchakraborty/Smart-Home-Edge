#ifndef MEMORYMAP_H
#define MEMORYMAP_H

#include <stdint.h>

// typedef struct
// {
//     uint16_t magic;     // must be EEPROM_MAGIC

//     /* -------- Digital Outputs -------- */
//     // uint16_t OUTPUTS;  
//     // // D0: LED TX 
//     // // D1: LED RX 
//     // // D2: Buzzer 
//     // // D3: Relay 1 
//     // // D4: Relay 2  
//     // uint16_t START_HH_MM;  
//     // uint16_t END_HH_MM;  
// } ModbusData_t;


/* -------- Global data (DECLARATION) -------- */
// extern ModbusData_t modbusData;

#define MODBUS_MEM_SIZE 200
#define MODBUS_PRODUCT_ID_FIRST 51
#define MODBUS_PRODUCT_ID_LAST 70
#define MODBUS_SERIAL_NUMBER_FIRST 71
#define MODBUS_SERIAL_NUMBER_LAST 90
extern uint16_t modbusMemory[MODBUS_MEM_SIZE];

void ModbusMemory_LoadTestPattern();
void ModbusMemory_LoadProductIdentity();
bool ModbusMemory_IsReadOnly(uint16_t address);

/* -------- API (DECLARATION ONLY) -------- */
// uint16_t modbusMemoryRead(uint16_t address);
// void     modbusMemoryWrite(uint16_t address, uint16_t value);





#endif // MEMORYMAP_H
