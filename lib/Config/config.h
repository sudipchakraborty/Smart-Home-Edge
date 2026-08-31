#ifndef APP_CONFIG_H
#define APP_CONFIG_H

// Bluetooth Wi-Fi provisioning settings.
#define WIFI_PROV_DEVICE_NAME "PROV_LPTM"
#define WIFI_PROV_POP         "lptm2026"
#define WIFI_PROV_FORCE_RESET false

// Indian Standard Time (UTC + 05:30).
#define INTERNET_TIME_GMT_OFFSET_SECONDS      19800L
#define INTERNET_TIME_DAYLIGHT_OFFSET_SECONDS 0

// Internet OTA. Leave URL empty to disable automatic update checks.
#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION "1.0.0"
#endif
#define OTA_VERSION_URL "https://github.com/sudipchakraborty/Low-Power-Timer-Module/releases/latest/download/manifest.json"
#define OTA_CHECK_INTERVAL_MS 21600000UL

#endif // APP_CONFIG_H
