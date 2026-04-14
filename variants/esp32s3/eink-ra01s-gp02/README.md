# eink-ra01s-gp02

Custom Meshtastic firmware variant for an ESP32-S3 board with an SX1262 LoRa radio,
a GP-02 GPS module, and an e-ink display.

## Hardware

| Component | Part                       | Notes                                  |
| --------- | -------------------------- | -------------------------------------- |
| MCU       | ESP32-S3-WROOM-1-N16R8     | 16 MB Flash, 8 MB Octal PSRAM          |
| LoRa      | Ai-Thinker RA-01S (SX1262) | 868 / 915 MHz, no TCXO, DIO2 RF switch |
| GPS       | Ai-Thinker GP-02           | L76K compatible, NMEA 9600 bps         |
| Display   | E-Ink (TBD)                | GxEPD2, model TBD                      |

## File Structure

```
variants/esp32s3/eink-ra01s-gp02/
├── variant.h         Board entry point — identity, HAS_* toggles, includes below
├── lora_sx1262.h     SX1262 SPI + control pin definitions
├── gps_gp02.h        GP-02 UART + optional control pin definitions
├── display_eink.h    E-ink panel + SPI pin definitions
├── peripherals.h     I2C, LED, NeoPixel, battery ADC
├── pins_arduino.h    Arduino framework default bus mappings (SPI, I2C, UART)
├── variant.cpp       Hardware init hooks (earlyInit / initVariant / lateInit)
└── platformio.ini    PlatformIO build environment
```

## Build.sh

```shell
chmod +x build.sh

ls /dev/* | grep -E "ttyACM*|ttyUSB*"

variants/esp32s3/eink-ra01s-gp02/build.sh --target upload  --upload-port  /dev/ttyACM0
variants/esp32s3/eink-ra01s-gp02/build.sh --target upload  --upload-port  /dev/ttyUSB0

variants/esp32s3/eink-ra01s-gp02/build.sh --target monitor --monitor-port /dev/ttyUSB0

variants/esp32s3/eink-ra01s-gp02/build.sh --target upload  --upload-port  /dev/ttyUSB0 --target monitor --monitor-port /dev/ttyUSB0
```

```bash
# compile_commands.json
variants/esp32s3/eink-ra01s-gp02/build.sh -t compiledb
```

```json
{
  "configurations": [
    {
      "name": "eink-ra01s-gp02",
      "compileCommands": "${workspaceFolder}/compile_commands.json"
    }
  ],
  "version": 4
}
```

## Build

```bash
pio run --verbose --environment eink-ra01s-gp02  2>&1 | tee .pio/logs/build-$(date +%Y%m%d-%H%M%S).log
```

```bash
pio run -e eink-ra01s-gp02
```

First-time flash (USB-CDC, esptool):

```bash
pio run -e eink-ra01s-gp02 -t upload
```

Serial monitor:

```bash
pio device monitor -e eink-ra01s-gp02
```

## Bring-up Checklist

Work through each section in order, building and testing after each step.

### 1. LoRa (required for mesh)

- [ ] Wire RA-01S to ESP32-S3 (SCK / MOSI / MISO / NSS / RESET / BUSY / DIO1)
- [ ] Fill in `LORA_SCK`, `LORA_MISO`, `LORA_MOSI`, `LORA_CS` in `lora_sx1262.h`
- [ ] Fill in `SX126X_RESET`, `SX126X_BUSY`, `SX126X_DIO1` in `lora_sx1262.h`
- [ ] Update `SS`, `MOSI`, `MISO`, `SCK` in `pins_arduino.h` to match
- [ ] Build and verify radio initializes (check serial log for SX1262 init message)

### 2. GPS

- [ ] Wire GP-02 TX → ESP32-S3 RX, GP-02 RX → ESP32-S3 TX
- [ ] Fill in `GPS_RX_PIN` and `GPS_TX_PIN` in `gps_gp02.h`
- [ ] Update `TX` / `RX` in `pins_arduino.h` to match
- [ ] Set `HAS_GPS = 1` in `variant.h`
- [ ] Build and verify NMEA sentences appear on serial

### 3. E-Ink display

- [ ] Identify GxEPD2 model class for your panel
- [ ] Wire display SPI + control pins
- [ ] Fill in all `PIN_EINK_*` defines in `display_eink.h`
- [ ] Set `EINK_DISPLAY_MODEL`, `EINK_WIDTH`, `EINK_HEIGHT`
- [ ] Uncomment `#define USE_EINK` in `display_eink.h`
- [ ] Uncomment GxEPD2 lib dependency in `platformio.ini`
- [ ] Set `HAS_SCREEN = 1` in `variant.h`
- [ ] Build and verify display renders the boot screen

### 4. Optional peripherals

- [ ] I2C sensor / RTC — fill in `I2C_SDA`, `I2C_SCL` in `peripherals.h`
- [ ] Battery ADC — fill in `BATTERY_PIN`, `ADC_MULTIPLIER` in `peripherals.h`
- [ ] Button — add `BUTTON_PIN` to `variant.h`, set `HAS_BUTTON = 1`
- [ ] NeoPixel — fill in `NEOPIXEL_DATA`, `NEOPIXEL_COUNT` in `peripherals.h`

## Known Limitations

- GP-02 `GNSS_SEL` (constellation selection) and `FORCE_ON` pins have no
  Meshtastic framework support; implement in `variant.cpp` if needed.
- `PRIVATE_HW` vendor ID is used; the node will show as "Private HW" in the app.
