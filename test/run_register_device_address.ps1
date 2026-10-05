$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$vcvars = Get-ChildItem 'C:\Program Files\Microsoft Visual Studio\*\*\VC\Auxiliary\Build\vcvars64.bat' -ErrorAction SilentlyContinue | Select-Object -First 1 -ExpandProperty FullName
if (!$vcvars) { throw 'Install the Visual Studio C++ build tools to run this host test.' }
$testOutput = Join-Path $env:TEMP 'smarthome-address-test'
New-Item -ItemType Directory -Force -Path $testOutput | Out-Null
$batchPath = Join-Path $testOutput 'run.cmd'
@"
@echo off
call "$vcvars" >nul
cl /nologo /EHsc /I "$root\lib\Modbus" /I "$root\lib\Memory" "$PSScriptRoot\register_device_address.cpp" /Fe:"$testOutput\address-test.exe" /Fo:"$testOutput\address-test.obj"
if errorlevel 1 exit /b 1
"$testOutput\address-test.exe"
exit /b %errorlevel%
"@ | Set-Content $batchPath
& $batchPath
if ($LASTEXITCODE -ne 0) { throw "Address behavior test failed: $LASTEXITCODE" }
Write-Host 'PASS: all 1000 IDs, EEPROM byte decoding, write acknowledgement at old address, invalid-write rejection, and new-address readback'
