# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(lora_sensor)

target_sources(app PRIVATE
    src/main.c
    src/lora.c
)

target_sources(app PRIVATE 
    src/drivers/sensors/sensors.c
    src/drivers/sensors/pms.c
    src/drivers/sensors/cm1106.c
    src/drivers/sensors/sht3x.c
    src/drivers/sensors/scd4x.c
    src/drivers/sensors/bme280.c
    src/drivers/sensors/sps30.c

    src/drivers/cayenne_lpp.c
    src/drivers/display.c
    src/drivers/led.c
)
target_include_directories(app PRIVATE
    src/drivers/
    src/drivers/sensors/
    ./src/
)

# add_subdirectory(src/LoRaMac)
# target_link_libraries(app PRIVATE loramac)