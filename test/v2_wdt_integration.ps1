$ErrorActionPreference = "Stop"

$config = Get-Content -Raw "lib/Config/config.h"
$source = Get-Content -Raw "src/PRJ_LPTM_V2.cpp"
$headerPath = "lib/WDT/WDTManager.h"
$sourcePath = "lib/WDT/WDTManager.cpp"

if (-not (Test-Path -LiteralPath $headerPath)) {
    throw "WDT module header is missing at lib/WDT/WDTManager.h."
}

if (-not (Test-Path -LiteralPath $sourcePath)) {
    throw "WDT module source is missing at lib/WDT/WDTManager.cpp."
}

if ($config -notmatch "WDT_ENABLED" -or
    $config -notmatch "WDT_TIMEOUT_SECONDS" -or
    $config -notmatch "WDT_TRIGGER_PANIC") {
    throw "WDT defaults must live in lib/Config/config.h."
}

if ($source -notmatch '#include\s+"WDTManager.h"') {
    throw "V2 project must include WDTManager.h."
}

if ($source -notmatch "wdt\.begin\s*\(") {
    throw "V2 setup must start the watchdog with passed parameters."
}

if ($source -notmatch "wdt\.feed\s*\(") {
    throw "V2 loop must feed the watchdog."
}

Write-Host "PASS: V2 watchdog module is configured and integrated."
