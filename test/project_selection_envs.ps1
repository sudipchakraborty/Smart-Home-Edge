$ErrorActionPreference = "Stop"

$platformio = Get-Content -Raw "platformio.ini"
$selection = Get-Content -Raw "include/ProjectSelection.h"

if ($platformio -notmatch "\[env:esp32doit-devkit-v1\][\s\S]*?build_flags\s*=\s*-D LowPowerTimerModule_V1") {
    throw "V1 PlatformIO environment must explicitly define LowPowerTimerModule_V1."
}

if ($platformio -notmatch "\[env:lptm-v2\][\s\S]*?build_flags\s*=\s*-D LowPowerTimerModule_V2") {
    throw "V2 PlatformIO environment must explicitly define LowPowerTimerModule_V2."
}

if ($selection -match "#define\s+LowPowerTimerModule_V2") {
    throw "ProjectSelection.h must not unconditionally define LowPowerTimerModule_V2."
}

if ($selection -notmatch "#error\s+`"Select only one Low Power Timer Module version`"") {
    throw "ProjectSelection.h must guard against selecting both V1 and V2."
}

Write-Host "PASS: PlatformIO environments own V1/V2 selection cleanly."
