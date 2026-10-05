$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
foreach ($file in @('src/PRJ_LPTM.cpp', 'src/PRJ_LPTM_V2.cpp')) {
    $source = Get-Content (Join-Path $root $file) -Raw
    if ($source -match 'MY_SLAVE_ID') { throw "$file still has a separate slave ID" }
    if ($source -notmatch 'ModbusDeviceAddress::read\(modbusMemory\)') { throw "$file does not filter from registers" }
    if ($source -notmatch 'ModbusDeviceAddress::decode\(deviceId\)') { throw "$file does not validate EEPROM address" }
    if ($source -notmatch 'if \(!validId\)' -or $source -notmatch 'if \(!validName\)') { throw "$file couples address loading to name validity" }
    if ($source -notmatch "txBuf\[3\] == '8'") { throw "$file applies exception writes" }
}
$builder = Get-Content (Join-Path $root 'lib/Modbus/modbusResponseBuilder.cpp') -Raw
if ($builder -notmatch 'ModbusDeviceAddress::validWrite') { throw 'Missing pre-mutation ID write validation' }
$helper = Get-Content (Join-Path $root 'lib/Modbus/ModbusDeviceAddress.h') -Raw
if ($helper -notmatch 'value >= 1 && value <= 247') { throw 'Missing unicast range validation' }
Write-Host 'PASS: V1/V2 register address, independent EEPROM loading, and invalid-write guards'
