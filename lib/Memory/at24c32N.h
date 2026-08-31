#ifndef AT24C32N_H
#define AT24C32N_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* -------- AT24C32 CONFIG -------- */

#ifndef AT24C32_I2C_ADDR
#define AT24C32_I2C_ADDR   0x57    // Onboard DS3231 module: A2=A1=A0=1
#endif
#define AT24C32_PAGE_SIZE  32      // bytes
#define AT24C32_TOTAL_BYTES 4096

/* -------- Public API -------- */

// Must be called once
void AT24C32_Init(uint8_t sclPin, uint8_t sdaPin, uint32_t freq);

// Single 16-bit register read
bool AT24C32_ReadRegister(uint16_t regAddr, uint16_t* value);

// Multiple 16-bit register read
bool AT24C32_ReadRegisters(uint16_t startReg,
                           uint16_t count,
                           uint16_t* buffer);

// Single 16-bit register write
bool AT24C32_WriteRegister(uint16_t regAddr, uint16_t value);

// Multiple 16-bit register write
bool AT24C32_WriteRegisters(uint16_t startReg,
                            uint16_t count,
                            const uint16_t* buffer);

                            // Add these to your public API section in at24c32N.h
bool i2cWrite(uint16_t memAddr, const uint8_t* data, size_t len);
bool i2cRead(uint16_t memAddr, uint8_t* data, size_t len);
int32_t AT24C32_Read3BytesAsInt(uint16_t startAddr);


#endif
