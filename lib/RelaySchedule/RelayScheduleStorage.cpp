#include "RelayScheduleStorage.h"

#include "Utils.h"

RelayScheduleStorage::RelayScheduleStorage(
    EEPROMStorage &storage,
    uint16_t *modbusMemory,
    const RelayScheduleDefinition *definitions,
    size_t definitionCount,
    DiagnosticCallback diagnostic)
    : _storage(storage),
      _modbusMemory(modbusMemory),
      _definitions(definitions),
      _definitionCount(definitionCount > MaxSchedules ? MaxSchedules : definitionCount),
      _diagnostic(diagnostic),
      _lastValid{},
      _receivedWordMask{},
      _pendingLowWord{},
      _pendingHighWord{}
{
}

void RelayScheduleStorage::begin()
{
    for (size_t i = 0; i < _definitionCount; ++i)
    {
        const uint32_t current = readModbusValue(i);
        _lastValid[i] = isValid(current) ? current : 0;
        writeModbusValue(i, _lastValid[i]);
        _receivedWordMask[i] = 0;
        _pendingLowWord[i] = static_cast<uint16_t>(_lastValid[i] & 0xFFFFUL);
        _pendingHighWord[i] = static_cast<uint16_t>(_lastValid[i] >> 16);
    }
}

bool RelayScheduleStorage::loadAll()
{
    bool allLoaded = true;
    for (size_t i = 0; i < _definitionCount; ++i)
    {
        uint32_t value = 0;
        if (!_storage.readUint32(_definitions[i].eepromAddress, value))
        {
            report(String("EEPROM READ ERROR: ") + _definitions[i].label +
                   ", address " + String(_definitions[i].eepromAddress));
            writeModbusValue(i, _lastValid[i]);
            allLoaded = false;
            continue;
        }

        if (!isValid(value))
        {
            report(String("INVALID SCHEDULE: ") + _definitions[i].label +
                   ", value " + String(value) + ", EEPROM address " +
                   String(_definitions[i].eepromAddress));
            writeModbusValue(i, _lastValid[i]);
            allLoaded = false;
            continue;
        }

        _lastValid[i] = value;
        writeModbusValue(i, value);
        _pendingLowWord[i] = static_cast<uint16_t>(value & 0xFFFFUL);
        _pendingHighWord[i] = static_cast<uint16_t>(value >> 16);
    }
    return allLoaded;
}

bool RelayScheduleStorage::persistAll()
{
    bool allSaved = true;
    for (size_t i = 0; i < _definitionCount; ++i)
    {
        const uint32_t value = readModbusValue(i);
        if (!isValid(value))
        {
            reportWords(i, _modbusMemory[_definitions[i].modbusStartAddress],
                        _modbusMemory[_definitions[i].modbusStartAddress + 1], value);
            report(String("INVALID SCHEDULE: ") + _definitions[i].label +
                   ", value " + String(value));
            writeModbusValue(i, _lastValid[i]);
            allSaved = false;
            continue;
        }

        if (!persistOne(i, value))
        {
            writeModbusValue(i, _lastValid[i]);
            allSaved = false;
        }
        else
        {
            _pendingLowWord[i] = static_cast<uint16_t>(value & 0xFFFFUL);
            _pendingHighWord[i] = static_cast<uint16_t>(value >> 16);
        }
    }
    return allSaved;
}

RelayScheduleStorage::WriteResult
RelayScheduleStorage::handleModbusWordWrite(uint16_t registerAddress)
{
    for (size_t i = 0; i < _definitionCount; ++i)
    {
        const uint16_t start = _definitions[i].modbusStartAddress;
        if (registerAddress != start && registerAddress != start + 1)
            continue;

        if (registerAddress == start)
        {
            _pendingLowWord[i] = _modbusMemory[start];
            _receivedWordMask[i] |= 0x01;
        }
        else
        {
            _pendingHighWord[i] = _modbusMemory[start + 1];
            _receivedWordMask[i] |= 0x02;
        }

        const uint32_t value =
            (static_cast<uint32_t>(_pendingHighWord[i]) << 16) |
            _pendingLowWord[i];
        reportWords(i, _pendingLowWord[i], _pendingHighWord[i], value);

        // The response builder has already copied this single word into the
        // public register map. Keep readers on the last complete valid pair
        // while the other word is still pending.
        writeModbusValue(i, _lastValid[i]);

        if (_receivedWordMask[i] != 0x03)
            return WriteResult::PendingPair;

        _receivedWordMask[i] = 0;
        if (!isValid(value))
        {
            report(String("INVALID SCHEDULE: ") + _definitions[i].label +
                   ", value " + String(value));
            writeModbusValue(i, _lastValid[i]);
            _pendingLowWord[i] = static_cast<uint16_t>(_lastValid[i] & 0xFFFFUL);
            _pendingHighWord[i] = static_cast<uint16_t>(_lastValid[i] >> 16);
            return WriteResult::Invalid;
        }

        if (!persistOne(i, value))
        {
            writeModbusValue(i, _lastValid[i]);
            _pendingLowWord[i] = static_cast<uint16_t>(_lastValid[i] & 0xFFFFUL);
            _pendingHighWord[i] = static_cast<uint16_t>(_lastValid[i] >> 16);
            return WriteResult::StorageError;
        }

        writeModbusValue(i, value);
        _pendingLowWord[i] = static_cast<uint16_t>(value & 0xFFFFUL);
        _pendingHighWord[i] = static_cast<uint16_t>(value >> 16);
        return WriteResult::Updated;
    }
    return WriteResult::NotSchedule;
}

bool RelayScheduleStorage::isValid(uint32_t secondsFromMidnight)
{
    return secondsFromMidnight <= 86399UL;
}

uint32_t RelayScheduleStorage::readModbusValue(size_t index) const
{
    const uint16_t start = _definitions[index].modbusStartAddress;
    const uint32_t low = _modbusMemory[start];
    const uint32_t high = _modbusMemory[start + 1];
    return (high << 16) | low;
}

void RelayScheduleStorage::writeModbusValue(size_t index, uint32_t value)
{
    const uint16_t start = _definitions[index].modbusStartAddress;
    _modbusMemory[start] = static_cast<uint16_t>(value & 0xFFFFUL);
    _modbusMemory[start + 1] = static_cast<uint16_t>((value >> 16) & 0xFFFFUL);
}

void RelayScheduleStorage::report(const String &message) const
{
    if (_diagnostic != nullptr)
        _diagnostic(message);
}

void RelayScheduleStorage::reportWords(size_t index, uint16_t lowWord,
                                       uint16_t highWord, uint32_t value) const
{
    report(String("RELAY SCHEDULE WORDS: ") + _definitions[index].label +
           ", low=" + String(lowWord) +
           ", high=" + String(highWord) +
           ", value=" + String(value));
}

bool RelayScheduleStorage::persistOne(size_t index, uint32_t value)
{
    if (!_storage.writeUint32(_definitions[index].eepromAddress, value))
    {
        report(String("EEPROM WRITE ERROR: ") + _definitions[index].label +
               ", address " + String(_definitions[index].eepromAddress));
        return false;
    }

    _lastValid[index] = value;
    report(String("RELAY SCHEDULE UPDATED: ") + _definitions[index].label +
           " " + Utils::secondsToHHMMSS(value));
    return true;
}
