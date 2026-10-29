# Hardware

Two revisions of the ESP32-S3 node were designed in 嘉立创EDA (LCEDA)
and fabricated at JLCPCB. Both are 2-layer boards.

## V1

- Display: SSD1306 128×64 (I2C)
- Particulate sensor: PMS7003 (UART, 5 V)
- Purpose: prove the full path — sensors → Zephyr → Cayenne LPP → LoRaWAN uplink

## V2

- Display: ST7789V 240×280 (SPI) with CST816S capacitive touch (I2C)
- Particulate sensor: SPS30 (I2C, 5 V)
- Purpose: local LVGL UI and a particulate sensor on I2C

## Files to add here

- `v1/schematic.png`, `v1/pcb.png`, `v1/board.jpg`
- `v2/schematic.png`, `v2/pcb.png`, `v2/board.jpg`, `v2/enclosure.stl`
- `v2/lceda-project.epro` (optional, export from 嘉立创EDA)

Keep the images under ~500 KB each so the repository stays light.
