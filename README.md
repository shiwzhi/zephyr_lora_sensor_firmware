# Multi-Sensor LoRaWAN Air Quality Node (Zephyr RTOS)

A multi-sensor environmental monitoring node running on Zephyr RTOS.
Reads CO2, PM1/PM2.5/PM10, temperature, humidity and pressure, encodes
the data with Cayenne LPP and reports it over LoRaWAN. Supports a local
LVGL/ST7789 UI, MCUboot firmware updates and a hardware watchdog.

**Demo:** [Bilibili — ESP32-S3 + Zephyr + LVGL + ST7789, custom PCB and enclosure](https://www.bilibili.com/video/BV16fnszrEqm)

---

## Features

- Multi-board support: RAK3172 (STM32WLE5), ESP32-C3, ESP32-S3, RP2040
- LoRaWAN Class A end device built on **LoRaMAC-node**, integrated as a
  Zephyr static library with the MAC callbacks wired to a thin wrapper layer
- Cayenne LPP payload encoding — decodes directly in TTN / ChirpStack
- Downlink calibration: a 4-byte float on FPort 62 sets the SCD4x
  temperature offset
- Local UI: LVGL tileview with colour-coded CO2 / PM thresholds on ST7789,
  or a lightweight CFB text fallback when LVGL is disabled
- MCUboot bootloader + sysbuild image configuration
- Independent watchdog thread (30 s window, fed every 15 s)
- Join auto-retry loop; 60 s measurement and report cycle

---

## Hardware

Two revisions of the ESP32-S3 node were designed in **嘉立创EDA (LCEDA)**
and fabricated at JLCPCB. Both are 2-layer boards, assembled and
validated on the bench.

| Rev | Display | Particulate sensor | Status |
|---|---|---|---|
| V1 | SSD1306 128×64 (I2C) | PMS7003 (UART, 5 V) | working — data path proven |
| V2 | ST7789V 240×280 (SPI) + CST816S touch (I2C) | SPS30 (I2C, 5 V) | working — LVGL UI |

V1 established the full path: sensors → Zephyr → LPP → LoRaWAN uplink.
V2 rebuilt the board around a larger SPI display with backlight and
touch, and moved the particulate sensor from UART (PMS7003) to I2C
(SPS30), which required a 5 V rail for the fan and a reworked layout.

### SMT assembly note (V1)

Two V1 boards were assembled by JLCPCB. On one of them the **BMP280 was
placed rotated 180°**. Power rails and I2C ACKs were normal, but the
readings were wrong. Comparing the physical part against the datasheet
pinout confirmed the rotation, and the board was sent back for rework.
The other board in the same batch was correct, which identified this as
a placement error at the assembly stage rather than a footprint error in
the design.

Schematics, PCB previews and build photos: [`docs/hardware/`](docs/hardware/).

---

## Supported boards

| Board | Radio | Display | Sensors wired |
|---|---|---|---|
| `rak3172` (STM32WLE5CC) | on-chip sub-GHz (RFO-HP, 22 dBm) | — | SCD41, BMP280, PMS (UART) |
| `esp32s3_devkitm_procpu` | SX1262 (SPI) | ST7789V 240×280 + CST816S touch | SCD40, BMP280, SPS30 |
| `esp32c3_devkitm` | SX1262 | SSD1306 128×64 | SCD40, BMP280, PMS (UART) |
| `esp32c3_luatos_core` | SX1262 | SSD1306 | SCD40, BMP280, PMS (UART) |
| `rpi_pico` | SX1262 | — | SCD40, BMP280 |

Board pinout, bus and radio configuration live in
`boards/<board>.overlay` — no board-conditional code in the application.

---

## Sensors

| Sensor | Measurement | Bus | Driver |
|---|---|---|---|
| SCD4x (SCD40/41) | CO2, temperature, humidity | I2C | `src/drivers/sensors/scd4x.c` |
| SPS30 | PM1 / PM2.5 / PM10 | I2C | `src/drivers/sensors/sps30.c` |
| PMSx003 | PM1 / PM2.5 / PM10 | UART 9600 | `src/drivers/sensors/pms.c` |
| CM1106 | CO2 | UART | `src/drivers/sensors/cm1106.c` |
| SHT3x | temperature, humidity | I2C | `src/drivers/sensors/sht3x.c` |
| BMP280 | pressure | I2C | `src/drivers/sensors/bme280.c` (shared BME280/BMP280 driver) |

Every driver exposes the same shape (`*_init` / `*_get*`) so `main.c`
does not care which sensor variant a board carries.

---

## Project layout

```
src/
├── main.c                 measurement + report loop, downlink handling
├── dog.c                  independent watchdog thread
├── display.c              LVGL UI (or CFB fallback)
├── drivers/
│   ├── cayenne_lpp.c      LPP encoder
│   └── sensors/           one file per sensor
└── lora/
    ├── lora.c             LoRaMAC-node wrapper: init, join, send, RX
    ├── lora.h
    └── CMakeLists.txt     builds LoRaMAC-node as a static library
boards/*.overlay           per-board devicetree (pins, buses, radio)
sysbuild.conf              enables MCUboot
```

---

## Payload format (Cayenne LPP)

| Type | Channel | Value |
|---|---|---|
| Analog output | 0 | PM1 (µg/m³) |
| Analog output | 1 | PM2.5 (µg/m³) |
| Analog output | 2 | PM10 (µg/m³) |
| Analog output | 3 | CO2 / 100 |
| Temperature | 0 | °C |
| Humidity | 0 | % |

Reported to FPort 1 as an unconfirmed uplink, about 30 bytes.

### Downlink

| FPort | Length | Meaning |
|---|---|---|
| 62 | 4 | IEEE-754 float, SCD4x temperature offset (°C) |

---

## Build

Requires a Zephyr (west) workspace and the LoRaMAC-node submodule:

```bash
git submodule update --init --recursive

west build -b rak3172 .                    # STM32WLE5 target
west build -b esp32s3_devkitm_procpu .     # display build
west build -b esp32c3_devkitm .
west flash
```

The LVGL UI is disabled by default. Enable the commented block in
`prj.conf` (`CONFIG_LVGL=y` plus the font options); otherwise
`display.c` falls back to the CFB text renderer.

MCUboot is enabled through `sysbuild.conf`
(`SB_CONFIG_BOOTLOADER_MCUBOOT=y`).

---

## LoRaWAN configuration

- **Region**: selected at compile time. Both **EU433** and **CN470** have
  been used in testing. The MAC is initialised with the region passed to
  `LoRaMacInitialization()` in `src/lora/lora.c`, and
  `src/lora/CMakeLists.txt` compiles only the matching region source
  (`RegionEU433.c` / `RegionCN470.c`) to keep flash usage down. The
  committed configuration is EU433; for CN470, swap the initialisation
  argument and the region source list.
- **CN470 / RP001 note**: CN470 follows the RP001 regional parameters,
  which is why this project pins **LoRaMAC-node v4.4.7**. Newer releases
  changed regional handling and did not match the target network.
- **Keys**: the DevEUI is read from `hwinfo_get_device_eui64()` and the
  AppKey is derived from it, so every board joins with a unique identity.
  **This is a development convenience, not secure provisioning** — replace
  it with keys programmed at production time.
- **Join**: OTAA, DR3, ADR on, retried every 6 s until accepted.

---

## Known limitations

Explicit about what is unfinished:

- **Region is compile-time only** — switching region means editing
  `lora.c` and the region source list, not a Kconfig option.
- **Duplicate LPP channels**: if two sensors of the same type are enabled
  (SPS30 + PMS, or SCD4x + SHT3x), the later write overwrites the earlier
  one on the same channel. Sensor selection is expected per board but is
  not enforced in code.
- **AppKey derived from DevEUI** — development only, not production security.
- **No OTA test coverage** — MCUboot is configured, but image update over
  the network is unverified.
- **No low-power measurement** — `CONFIG_PM` is present but not enabled
  or validated.
- ADR uses the stack default; no custom adaptation policy.

---

## What I wrote vs upstream

- **Mine**: sensor drivers, application logic, Cayenne LPP integration,
  board overlays, display / UI layer, watchdog thread, LoRaMAC-node
  wrapper and build integration, and the ESP32-S3 PCB (schematic and
  layout in 嘉立创EDA).
- **Upstream**: LoRaMAC-node (BSD-3-Clause) provides the MAC layer; the
  project skeleton follows Zephyr's LoRa sample.

---

## License

Apache-2.0 for this repository's own code — see [LICENSE](LICENSE).
LoRaMAC-node remains under its original BSD-3-Clause license.
