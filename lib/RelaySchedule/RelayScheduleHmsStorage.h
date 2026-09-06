#ifndef RELAY_SCHEDULE_HMS_STORAGE_H
#define RELAY_SCHEDULE_HMS_STORAGE_H

#include <Arduino.h>
#include <stddef.h>
#include <stdint.h>

#include "eepromStorage.h"

struct RelayScheduleHmsDefinition
{
    const char *label;
    uint16_t hourRegister;
    uint16_t minuteRegister;
    uint16_t secondRegister;
    uint16_t eepromAddress;
};

class RelayScheduleHmsStorage
{
public:
    enum class WriteResult
    {
        NotSchedule,
        Updated,
        Invalid,
        StorageError
    };

    using DiagnosticCallback = void (*)(const String &message);

    RelayScheduleHmsStorage(EEPROMStorage &storage,
                            uint16_t *modbusMemory,
                            const RelayScheduleHmsDefinition *definitions,
                            size_t definitionCount,
                            DiagnosticCallback diagnostic = nullptr);

    void begin();
    bool loadAll();
    bool persistAll();
    WriteResult handleModbusWrite(uint16_t registerAddress);

    uint32_t secondsAt(size_t index) const;
    static bool isValidHms(uint16_t hour, uint16_t minute, uint16_t second);

private:
    static constexpr size_t MaxSchedules = 8;

    EEPROMStorage &_storage;
    uint16_t *_modbusMemory;
    const RelayScheduleHmsDefinition *_definitions;
    size_t _definitionCount;
    DiagnosticCallback _diagnostic;
    uint32_t _lastValid[MaxSchedules];

    bool findDefinition(uint16_t registerAddress, size_t &index) const;
    uint32_t readSeconds(size_t index) const;
    void writeSeconds(size_t index, uint32_t value);
    bool persistOne(size_t index, uint32_t value);
    void report(const String &message) const;
};

#endif
