## 2026-09-24 - Product ID and serial number exposed through Modbus

- Added read-only ASCII identity registers to the shared Modbus holding-register map.
- Product ID: `LPTM-V1.0`, registers `51..70` (one ASCII character per register; unused positions are spaces).
- Product serial number: `IN-001-2609=0001`, registers `71..90` (one ASCII character per register; unused positions are spaces).
- The revised register table removes the overlap: Product ID uses `51..70`, and Product Serial Number uses `71..90`. Unused Product ID registers `60..70` and serial registers `87..90` are filled with spaces.
- Modbus function `03` can read these registers. Modbus function `06` returns exception code `02` for attempts to write either identity range.
- Changes: `lib/Memory/memoryMap.h`, `lib/Memory/memoryMap.cpp`, `lib/Modbus/modbusResponseBuilder.cpp`, `src/PRJ_LPTM.cpp`.
- Initial PlatformIO build found and fixed a missing `<cstddef>` include for `size_t`; the corrected build passed. Hardware/ModScan readback remains pending.
- PlatformIO build: PASS (platformio run -e esp32doit-devkit-v1).

## 2026-09-24 - Startup identity load and Modbus read path

- Confirmed the startup sequence loads both configured identity strings into `modbusMemory` before the RS485/Modbus handler is used.
- Each character is stored as its ASCII value in one holding register; unused registers are space-padded.
- Existing function `03` handling reads `modbusMemory`, so clients can fetch Product ID registers `51..70` and serial registers `71..90` directly.
- No Modbus master/self-read is required: this firmware is the Modbus slave/server and serves the startup-loaded values to the external client.
- The prior PlatformIO build remains the source-level validation; physical Modbus readback is still pending.

## 2026-09-24 - Revised non-overlapping identity pattern

- Updated the serial string to match the revised table exactly: register `82` contains `=` (`IN-001-2609=0001`).
- Confirmed unused mapped locations remain space-filled for future pattern changes.

## 2026-09-24 - Writable Device ID and Device Name with EEPROM persistence

- Added read/write Modbus registers `29..31` for the three-character Device ID and `32..50` for the Device Name.
- Added dedicated EEPROM storage at byte offsets `128..130` (Device ID) and `132..150` (Device Name), separate from relay/RTC data.
- Startup reads both fields from EEPROM into the Modbus holding-register map. If EEPROM data is unavailable or invalid, defaults `001` and `BATHROOM` are loaded.
- Modbus function `06` updates the register and immediately persists the complete Device ID/Name fields to EEPROM.
- Unused Device Name positions are space-filled and preserved for future naming-pattern changes.
- Startup identity-load implementation build: PASS.
- Revised identity pattern build: PASS.
- Device ID/name EEPROM persistence build: PASS.

## 2026-09-24 - Commit and publication

- Repository was synchronized with `origin/main` before publication.
- The Modbus identity and EEPROM persistence changes were committed and pushed from `main`.
## 2026-09-27 - Modbus reset command and relay status registers

- Added reset command registers `91..95`, accepting the ASCII sequence `R`, `E`, `S`, `E`, `T` through Modbus function `06`.
- Added read-only relay status registers `96` and `97`, returning ASCII `O` when Relay 1/2 is active and `F` otherwise.
- Implemented the map in both Low Power Timer Module V1 and V2 handlers; PlatformIO build verification and hardware readback remain separate checks.
## 2026-09-27 - Fix V2 relay timer project selection

- Found that the `lptm-v2` PlatformIO environment defined V2 in build flags while `include/ProjectSelection.h` unconditionally defined V1.
- This compiled both relay-timer projects together, preventing the frontend-backed V2 Modbus/EEPROM save path from building.
- Planned change: make the default V1 selection conditional so the V2 build flag selects only V2.

## 2026-09-27 - Validate V1 relay schedule save triggers

- V1 uses Modbus register `7` to save Relay 1 and register `14` to save Relay 2.
- The trigger handlers previously returned success without checking EEPROM availability or write status.
- Planned change: validate HH:MM:SS fields and report the actual EEPROM save result.

## 2026-09-27 - Planned V1/V2 shared Modbus register map alignment

- The supplied `Modbus Register Map.pptx` is the source of truth for both firmware variants.
- Align V2 relay schedules with the shared HH/MM/SS map: Relay 1 `1..6` with trigger `7`, Relay 2 `8..13` with trigger `14`.
- Preserve the shared RTC map `15..27`, output register `0`, identity registers `29..90`, reset command `91..95`, and relay status registers `96..97`.
- Remove V2-only packed 32-bit schedule assumptions and verify both PlatformIO environments.

## 2026-10-04 - Disable V2 WiFi setup hotspot server

- LowPowerTimerModule_V2 hardware does not provide the mobile webpage WiFi configuration feature.
- Removed the V2 `WiFiSetupServer` startup and loop handling that created the `192.168.4.1` setup hotspot.
- Removed the V2 hotspot credential defines from `src/PRJ_LPTM_V2.h`.
- Added `test/v2_no_wifi_setup_hotspot.ps1` to keep the V2 setup-hotspot path disabled.
- Cleaned project selection so PlatformIO environments explicitly define V1 or V2, and `ProjectSelection.h` only validates the selection.

## 2026-10-04 - Add V2 watchdog timer module

- Added reusable `lib/WDT` watchdog wrapper around ESP32 task watchdog APIs.
- Added watchdog defaults in `lib/Config/config.h`; no JSON configuration file is required for now.
- LowPowerTimerModule_V2 starts the watchdog after startup/system test and feeds it once per loop.
- Added `test/v2_wdt_integration.ps1` to verify the module/config/V2 wiring.

## 2026-10-05 - Align V2 identity registers with V1/frontend contract

- Found V2 only loaded writable Device ID registers `29..31`; Device Name `32..50`, Product ID `51..70`, and Serial No. `71..90` were not populated like V1.
- Added V2 Device Name register definitions and EEPROM storage at byte offset `132`.
- V2 now loads product/serial read-only registers at startup and reads/saves full Device ID/name identity data using the V1-compatible register map.
- Added `test/v2_identity_contract.ps1` to guard the frontend-visible identity contract.
- Updated the watchdog wrapper to use the ESP32 Arduino 2.x watchdog API used by this project.
- Verified both V1 and V2 PlatformIO builds, plus the project-selection, hotspot-removal, and watchdog checks.

## 2026-10-04 - Persist V2 Device ID registers

- Confirmed V2 had no Device ID register mapping or EEPROM load/write handling, although V1 uses Modbus registers `29..31` and EEPROM byte offset `128`.
- Added the same V2 Device ID holding registers (`29..31`) and EEPROM offset (`128`) as the shared device identity mapping.
- V2 startup now loads the three printable ASCII ID characters from EEPROM into `modbusMemory`; if the EEPROM read fails or data is invalid, the map uses default ID `001`.
- Modbus function `06` writes to registers `29..31` now persist the complete three-character ID. Invalid characters or EEPROM write errors restore the previous in-memory character and log the failure.
- Function `03` continues to return those values from `modbusMemory`; the three-character Device ID is distinct from the frame-level Modbus slave address, which remains unchanged.
- Added `test/v2_device_id_eeprom.ps1` to check register mapping, EEPROM load/save wiring, and the Modbus memory read path.
- Verification passed: the Device ID, project-selection, hotspot-removal, and watchdog checks, plus PlatformIO builds for both V2 and V1. EEPROM readback on hardware remains unverified.

## 2026-10-05 - Planned register-backed Modbus address (V1 and V2)

- Registers 29..31 (three ASCII digits) are the single source of the active Modbus address; remove independent MY_SLAVE_ID variables.
- Restore the address from EEPROM offset 128 at startup. Validate 001..247 independently of the device name; use 001 (V1) or 002 (V2) when invalid/unavailable.
- Add shared address decoding/write validation, use it for packet filtering, reject invalid address writes without changing memory/EEPROM, and preserve the old address in the write acknowledgement.
- Add regression checks, build both environments sequentially, and record outcomes here.
- Onsite writes take effect immediately. A client must use the current decoded address for every following write/read; existing backend identity updates still target the original unitId and require a separate follow-up change.

### Completed and verified

- V1/V2 packet filtering now decodes the address directly from ASCII registers 29..31; no independent MY_SLAVE_ID remains.
- Startup restores valid EEPROM address bytes independently of name validity. Fallback addresses are V1=001 and V2=002; existing saved 001 on V2 is preserved and now means address 1.
- Function 06 validates the candidate full ID before changing memory. Invalid/nondecimal/zero/>247 IDs return illegal-data-value and cannot reach EEPROM persistence. Accepted writes echo the request address and following packets use the updated register value.
- Existing EEPROM save handlers persist accepted onsite ID/name writes. Hardware EEPROM write/readback and power-cycle verification remain pending; firmware has not been uploaded.
- Fixed default-name padding to avoid reading beyond the BATHROOM string during startup fallback.
- Host behavior test: test/run_register_device_address.ps1 passed for all 1000 ASCII combinations, EEPROM byte decoding, old-address acknowledgement, rejection without mutation, and new-address register readback using the actual response builder.
- Structural checks passed: register_device_address.ps1, v2_device_id_eeprom.ps1, v2_identity_contract.ps1. git diff --check passed.
- Final sequential PlatformIO builds passed: esp32doit-devkit-v1 and lptm-v2.
- Client limitation: address changes take effect after each accepted single-register write. For IDs requiring multiple digit changes, send subsequent requests to each resulting intermediate address and keep intermediate IDs within 001..247. The current backend update route does not yet follow changed addresses; do not treat its Update device flow as verified for address changes.
