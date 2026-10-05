$ErrorActionPreference = "Stop"

$header = Get-Content -Raw "src/PRJ_LPTM_V2.h"
$source = Get-Content -Raw "src/PRJ_LPTM_V2.cpp"

if ($header -notmatch "ModbusAddr_DeviceId_First\s+29" -or
    $header -notmatch "ModbusAddr_DeviceId_Last\s+31" -or
    $header -notmatch "ModbusAddr_DeviceName_First\s+32" -or
    $header -notmatch "ModbusAddr_DeviceName_Last\s+50") {
    throw "V2 must expose the same Device ID and Device Name registers as V1."
}

if ($header -notmatch "EEPROM_Addr_DeviceId\s+128" -or
    $header -notmatch "EEPROM_Addr_DeviceName\s+132") {
    throw "V2 must reserve EEPROM storage for both Device ID and Device Name."
}

if ($source -notmatch "ModbusMemory_LoadProductIdentity\s*\(") {
    throw "V2 setup must load Product ID and Serial No. into read-only Modbus registers."
}

if ($source -notmatch "LoadDeviceIdentityFromEEPROM\s*\(" -or
    $source -notmatch "SaveDeviceIdentityToEEPROM\s*\(") {
    throw "V2 must load and save the full Device ID/name identity contract."
}

if ($source -notmatch "ModbusAddr_DeviceName_First\s*\.\.\.\s*ModbusAddr_DeviceName_Last") {
    throw "V2 Modbus write handler must accept Device Name registers."
}

Write-Host "PASS: V2 identity registers match the V1/frontend contract."
