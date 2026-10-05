$ErrorActionPreference = "Stop"

$header = Get-Content -Raw "src/PRJ_LPTM_V2.h"
$source = Get-Content -Raw "src/PRJ_LPTM_V2.cpp"
$responseBuilder = Get-Content -Raw "lib/Modbus/modbusResponseBuilder.cpp"

if ($header -notmatch "#define\s+ModbusAddr_DeviceId_First\s+29" -or
    $header -notmatch "#define\s+ModbusAddr_DeviceId_Last\s+31" -or
    $header -notmatch "#define\s+ModbusAddr_DeviceName_First\s+32" -or
    $header -notmatch "#define\s+ModbusAddr_DeviceName_Last\s+50" -or
    $header -notmatch "#define\s+EEPROM_Addr_DeviceId\s+128" -or
    $header -notmatch "#define\s+EEPROM_Addr_DeviceName\s+132") {
    throw "V2 Device ID/name registers and EEPROM offsets must be defined."
}

if ($source -notmatch "eeprom\.readBytes\(EEPROM_Addr_DeviceId,\s*deviceId,\s*sizeof\(deviceId\)\)") {
    throw "V2 startup must load the Device ID from EEPROM."
}

if ($source -notmatch "eeprom\.readBytes\(EEPROM_Addr_DeviceName,\s*deviceName,\s*sizeof\(deviceName\)\)") {
    throw "V2 startup must load the Device Name from EEPROM."
}

if ($source -notmatch "modbusMemory\[ModbusAddr_DeviceId_First\s*\+\s*i\]\s*=\s*deviceId\[i\]") {
    throw "The EEPROM Device ID must be copied into Modbus registers 29-31."
}

if ($source -notmatch "modbusMemory\[ModbusAddr_DeviceName_First\s*\+\s*i\]\s*=\s*deviceName\[i\]") {
    throw "The EEPROM Device Name must be copied into Modbus registers 32-50."
}

if ($source -notmatch "case\s+ModbusAddr_DeviceId_First\s*\.\.\.\s*ModbusAddr_DeviceId_Last:[\s\S]*?case\s+ModbusAddr_DeviceName_First\s*\.\.\.\s*ModbusAddr_DeviceName_Last:[\s\S]*?SaveDeviceIdentityToEEPROM\(\)") {
    throw "Writes to Device ID/name registers must be persisted to EEPROM."
}

if ($source -notmatch "eeprom\.writeBytes\(EEPROM_Addr_DeviceId,\s*deviceId,\s*sizeof\(deviceId\)\)[\s\S]*?eeprom\.writeBytes\(EEPROM_Addr_DeviceName,\s*deviceName,\s*sizeof\(deviceName\)\)") {
    throw "Device ID/name changes must save both fields to EEPROM."
}

if ($responseBuilder -notmatch "modbusMemory\[frame->address\s*\+\s*i\]") {
    throw "Function 03 must return Device ID values from Modbus memory."
}

Write-Host "PASS: V2 Device ID/name registers load from and save to EEPROM."
