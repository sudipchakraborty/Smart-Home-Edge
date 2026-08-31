#ifndef RELAY_SCHEDULE_STORAGE_H
#define RELAY_SCHEDULE_STORAGE_H

#include <Arduino.h>
#include <stddef.h>
#include <stdint.h>

#include "eepromStorage.h"

struct RelayScheduleDefinition
{
    const char *label;
    uint16_t modbusStartAddress;
    uint16_t eepromAddress;
};

class RelayScheduleStorage
{
public:
    enum class WriteResult
    {
        NotSchedule,
        PendingPair,
        Updated,
        Invalid,
        StorageError
    };

    using DiagnosticCallback = void (*)(const String &message);

    RelayScheduleStorage(EEPROMStorage &storage,
                         uint16_t *modbusMemory,
                         const RelayScheduleDefinition *definitions,
                         size_t definitionCount,
                         DiagnosticCallback diagnostic = nullptr);

    void begin();
    bool loadAll();
    bool persistAll();
    WriteResult handleModbusWordWrite(uint16_t registerAddress);

    static bool isValid(uint32_t secondsFromMidnight);

private:
    static constexpr size_t MaxSchedules = 8;

    EEPROMStorage &_storage;
    uint16_t *_modbusMemory;
    const RelayScheduleDefinition *_definitions;
    size_t _definitionCount;
    DiagnosticCallback _diagnostic;
    uint32_t _lastValid[MaxSchedules];
    uint8_t _receivedWordMask[MaxSchedules];
    uint16_t _pendingLowWord[MaxSchedules];
    uint16_t _pendingHighWord[MaxSchedules];

    uint32_t readModbusValue(size_t index) const;
    void writeModbusValue(size_t index, uint32_t value);
    void report(const String &message) const;
    void reportWords(size_t index, uint16_t lowWord,
                     uint16_t highWord, uint32_t value) const;
    bool persistOne(size_t index, uint32_t value);
};

#endif
