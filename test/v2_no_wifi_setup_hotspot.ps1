$ErrorActionPreference = "Stop"

$source = Get-Content -Raw "src/PRJ_LPTM_V2.cpp"
$header = Get-Content -Raw "src/PRJ_LPTM_V2.h"

if ($source -match "WiFiSetupServer") {
    throw "V2 firmware must not include, instantiate, start, or service WiFiSetupServer."
}

if ($source -match "WiFi setup hotspot|Factory hotspot password|WiFi setup page") {
    throw "V2 firmware must not advertise the mobile WiFi setup hotspot."
}

if ($header -match "Hotspot_Name|Hotspot_password|Wi-Fi setup hotspot") {
    throw "V2 header must not expose WiFi setup hotspot credentials."
}

Write-Host "PASS: V2 WiFi setup hotspot path is disabled."
