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
