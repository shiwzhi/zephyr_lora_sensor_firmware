cmake_minimum_required(VERSION 3.20.0)

add_library(loramac)

target_sources(loramac PRIVATE 
    LoRaMac-node/src/mac/LoRaMac.c
    LoRaMac-node/src/mac/LoRaMacClassB.c
    LoRaMac-node/src/mac/LoRaMacCrypto.c
    LoRaMac-node/src/mac/LoRaMacCommands.c
    LoRaMac-node/src/mac/LoRaMacSerializer.c
    LoRaMac-node/src/mac/LoRaMacConfirmQueue.c
    LoRaMac-node/src/mac/LoRaMacAdr.c
    LoRaMac-node/src/mac/LoRaMacParser.c

    LoRaMac-node/src/mac/region/Region.c
    LoRaMac-node/src/mac/region/RegionCommon.c
    loramac-node/src/mac/region/RegionEU433.c
    # LoRaMac-node/src/mac/region/RegionCN470.c
    # LoRaMac-node/src/mac/region/RegionBaseUS.c


    LoRaMac-node/src/peripherals/soft-se/soft-se.c
    LoRaMac-node/src/peripherals/soft-se/soft-se-hal.c
    LoRaMac-node/src/peripherals/soft-se/aes.c
    LoRaMac-node/src/peripherals/soft-se/cmac.c

    LoRaMac-node/src/system/nvmm.c

    # use zephyr implementation
    # LoRaMac-node/src/system/delay.c
    # LoRaMac-node/src/system/timer.c
    # LoRaMac-node/src/system/systime.c
    # LoRaMac-node/src/boards/mcu/utilities.c
    # LoRaMac-node/src/radio/sx126x/radio.c
    # LoRaMac-node/src/radio/sx126x/sx126x.c
)

target_include_directories(loramac PUBLIC
    LoRaMac-node/src/peripherals/soft-se
    LoRaMac-node/src/radio
    LoRaMac-node/src/radio/sx126x
    LoRaMac-node/src/mac
    LoRaMac-node/src/system
    LoRaMac-node/src/mac/region
    LoRaMac-node/src/boards
)

target_link_libraries(loramac PUBLIC zephyr)
target_compile_definitions(loramac PRIVATE REGION_EU433=1 SOFT_SE=1)