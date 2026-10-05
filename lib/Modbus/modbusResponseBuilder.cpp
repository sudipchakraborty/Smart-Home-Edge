#include "modbusResponseBuilder.h"
#include "memoryMap.h"
#include "ModbusDeviceAddress.h"

/* ---------------- Internal helpers ---------------- */

static uint8_t bin[MODBUS_BIN_MAX];   // ✅ static (safe)

static inline char toHex(uint8_t v)
{
    return (v < 10) ? ('0' + v) : ('A' + v - 10);
}

static void byteToHex(uint8_t b, char* out)
{
    out[0] = toHex((b >> 4) & 0x0F);
    out[1] = toHex(b & 0x0F);
}

static uint8_t calcLRC(const uint8_t* data, uint8_t len)
{
    uint8_t lrc = 0;
    for (uint8_t i = 0; i < len; i++)
        lrc += data[i];
    return (uint8_t)(-((int8_t)lrc));
}

static char buildASCII(const uint8_t* bin, uint8_t binLen, char* out, size_t outSize)
{
    size_t needed = 1 + (binLen + 1) * 2 + 2 + 1;

    if (outSize < needed)
        return 0;

    size_t idx = 0;
    out[idx++] = ':';

    for (uint8_t i = 0; i < binLen; i++)
    {
        byteToHex(bin[i], &out[idx]);
        idx += 2;
    }

    uint8_t lrc = calcLRC(bin, binLen);
    byteToHex(lrc, &out[idx]);
    idx += 2;

    out[idx++] = '\r';
    out[idx++] = '\n';
    out[idx]   = '\0';

    return (char)idx;
}

static char buildException(uint8_t slave, uint8_t func, uint8_t code,
                           char* out, size_t outSize)
{
    uint8_t tmp[3];
    tmp[0] = slave;
    tmp[1] = func | 0x80;
    tmp[2] = code;

    return buildASCII(tmp, 3, out, outSize);
}

/* ---------------- Main dispatcher ---------------- */

char Modbus_BuildResponse(const ModbusASCIIFrame* frame,
                          char* responseBuffer,
                          size_t bufferSize)
{
    if (!frame || !frame->valid)
        return 0;

    if (frame->slaveId == 0)   // broadcast
        return 0;

    uint16_t idx = 0;

    switch (frame->function)
    {
        case 0x03:
        {
            // ✅ Quantity validation (CRITICAL)
            if (frame->quantity == 0 || frame->quantity > MODBUS_MAX_REGS)
                return buildException(frame->slaveId, 0x03, 0x03, responseBuffer, bufferSize);

            // ✅ Address validation
            if ((frame->address + frame->quantity) > MODBUS_MEM_SIZE)
                return buildException(frame->slaveId, 0x03, 0x02, responseBuffer, bufferSize);

            // ✅ Buffer overflow protection
            uint16_t needed = 3 + frame->quantity * 2;
            if (needed > MODBUS_BIN_MAX)
                return buildException(frame->slaveId, 0x03, 0x03, responseBuffer, bufferSize);

            bin[idx++] = frame->slaveId;
            bin[idx++] = 0x03;
            bin[idx++] = frame->quantity * 2;

            for (uint16_t i = 0; i < frame->quantity; i++)
            {
                uint16_t val = modbusMemory[frame->address + i];
                bin[idx++] = val >> 8;
                bin[idx++] = val & 0xFF;
            }

            return buildASCII(bin, idx, responseBuffer, bufferSize);
        }

        case 0x06:
        {
            if (frame->address >= MODBUS_MEM_SIZE)
                return buildException(frame->slaveId, 0x06, 0x02, responseBuffer, bufferSize);

            if (ModbusMemory_IsReadOnly(frame->address))
                return buildException(frame->slaveId, 0x06, 0x02, responseBuffer, bufferSize);

            if (!ModbusDeviceAddress::validWrite(modbusMemory, frame->address, frame->value))
                return buildException(frame->slaveId, 0x06, 0x03, responseBuffer, bufferSize);

            modbusMemory[frame->address] = frame->value;

            bin[0] = frame->slaveId;
            bin[1] = 0x06;
            bin[2] = frame->address >> 8;
            bin[3] = frame->address & 0xFF;
            bin[4] = frame->value >> 8;
            bin[5] = frame->value & 0xFF;

            return buildASCII(bin, 6, responseBuffer, bufferSize);
        }

        case 0x10:
        {
            if (frame->quantity == 0 || frame->quantity > MODBUS_MAX_REGS)
                return buildException(frame->slaveId, 0x10, 0x03, responseBuffer, bufferSize);

            if ((frame->address + frame->quantity) > MODBUS_MEM_SIZE)
                return buildException(frame->slaveId, 0x10, 0x02, responseBuffer, bufferSize);

            // (write logic if implemented)

            bin[0] = frame->slaveId;
            bin[1] = 0x10;
            bin[2] = frame->address >> 8;
            bin[3] = frame->address & 0xFF;
            bin[4] = frame->quantity >> 8;
            bin[5] = frame->quantity & 0xFF;

            return buildASCII(bin, 6, responseBuffer, bufferSize);
        }

        default:
            return buildException(frame->slaveId, frame->function, 0x01, responseBuffer, bufferSize);
    }
}
