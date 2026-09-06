#include "RelayScheduleHmsStorage.h"

#include "Utils.h"

RelayScheduleHmsStorage::RelayScheduleHmsStorage(
    EEPROMStorage &storage,
    uint16_t *modbusMemory,
    const RelayScheduleHmsDefinition *definitions,
    size_t definitionCount,
    DiagnosticCallback diagnostic)
    : _storage(storage),
      _modbusMemory(modbusMemory),
      _definitions(definitions),
      _definitionCount(definitionCount > MaxSchedules ? MaxSchedules : definitionCount),
      _diagnostic(diagnostic),
      _lastValid{}
{
}

void RelayScheduleHmsStorage::begin()
{
    for (size_t i = 0; i < _definitionCount; ++i)
    {
        _lastValid[i] = 0;
        writeSeconds(i, 0);
    }
}

bool RelayScheduleHmsStorage::loadAll()
{
    bool allLoaded = true;
    for (size_t i = 0; i < _definitionCount; ++i)
    {
        uint32_t value = 0;
        if (!_storage.readUint32(_definitions[i].eepromAddress, value))
        {
            report(String("EEPROM READ ERROR: ") + _definitions[i].label);
            writeSeconds(i, _lastValid[i]);
            allLoaded = false;
            continue;
        }

        if (value > 86399UL)
        {
            report(String("INVALID SCHEDULE: ") + _definitions[i].label +
                   ", value " + String(value));
            writeSeconds(i, _lastValid[i]);
            allLoaded = false;
            continue;
        }

        _lastValid[i] = value;
        writeSeconds(i, value);
    }
    return allLoaded;
}

bool RelayScheduleHmsStorage::persistAll()
{
    bool allSaved = true;
    for (size_t i = 0; i < _definitionCount; ++i)
    {
        const uint32_t value = readSeconds(i);
        if (value > 86399UL || !persistOne(i, value))
        {
            writeSeconds(i, _lastValid[i]);
            allSaved = false;
        }
    }
    return allSaved;
}

RelayScheduleHmsStorage::WriteResult
RelayScheduleHmsStorage::handleModbusWrite(uint16_t registerAddress)
{
    size_t index = 0;
    if (!findDefinition(registerAddress, index))
        return WriteResult::NotSchedule;

    const RelayScheduleHmsDefinition &definition = _definitions[index];
    const uint16_t hour = _modbusMemory[definition.hourRegister];
    const uint16_t minute = _modbusMemory[definition.minuteRegister];
    const uint16_t second = _modbusMemory[definition.secondRegister];

    if (!isValidHms(hour, minute, second))
    {
        report(String("INVALID SCHEDULE: ") + definition.label +
               ", HH=" + String(hour) + ", MM=" + String(minute) +
               ", SS=" + String(second));
        writeSeconds(index, _lastValid[index]);
        return WriteResult::Invalid;
    }

    const uint32_t value = static_cast<uint32_t>(hour) * 3600UL +
                           static_cast<uint32_t>(minute) * 60UL + second;
    if (!persistOne(index, value))
    {
        writeSeconds(index, _lastValid[index]);
        return WriteResult::StorageError;
    }

    return WriteResult::Updated;
}

uint32_t RelayScheduleHmsStorage::secondsAt(size_t index) const
{
    return index < _definitionCount ? readSeconds(index) : 0;
}

bool RelayScheduleHmsStorage::isValidHms(uint16_t hour,
                                         uint16_t minute,
                                         uint16_t second)
{
    return hour <= 23 && minute <= 59 && second <= 59;
}

bool RelayScheduleHmsStorage::findDefinition(uint16_t registerAddress,
                                              size_t &index) const
{
    for (size_t i = 0; i < _definitionCount; ++i)
    {
        const RelayScheduleHmsDefinition &definition = _definitions[i];
        if (registerAddress == definition.hourRegister ||
            registerAddress == definition.minuteRegister ||
            registerAddress == definition.secondRegister)
        {
            index = i;
            return true;
        }
    }
    return false;
}

uint32_t RelayScheduleHmsStorage::readSeconds(size_t index) const
{
    const RelayScheduleHmsDefinition &definition = _definitions[index];
    const uint16_t hour = _modbusMemory[definition.hourRegister];
    const uint16_t minute = _modbusMemory[definition.minuteRegister];
    const uint16_t second = _modbusMemory[definition.secondRegister];
    if (!isValidHms(hour, minute, second))
        return 86400UL;
    return static_cast<uint32_t>(hour) * 3600UL +
           static_cast<uint32_t>(minute) * 60UL + second;
}

void RelayScheduleHmsStorage::writeSeconds(size_t index, uint32_t value)
{
    const RelayScheduleHmsDefinition &definition = _definitions[index];
    _modbusMemory[definition.hourRegister] = value / 3600UL;
    _modbusMemory[definition.minuteRegister] = (value % 3600UL) / 60UL;
    _modbusMemory[definition.secondRegister] = value % 60UL;
}

bool RelayScheduleHmsStorage::persistOne(size_t index, uint32_t value)
{
    if (!_storage.writeUint32(_definitions[index].eepromAddress, value))
    {
        report(String("EEPROM WRITE ERROR: ") + _definitions[index].label);
        return false;
    }

    _lastValid[index] = value;
    report(String("RELAY SCHEDULE UPDATED: ") + _definitions[index].label +
           " " + Utils::secondsToHHMMSS(value));
    return true;
}

void RelayScheduleHmsStorage::report(const String &message) const
{
    if (_diagnostic != nullptr)
        _diagnostic(message);
}
