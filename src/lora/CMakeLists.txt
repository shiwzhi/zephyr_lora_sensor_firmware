cmake_minimum_required(VERSION 3.6)

add_library(loramac STATIC)
target_link_libraries(loramac PUBLIC zephyr_interface)

target_include_directories(loramac PUBLIC LoRaMac-node/src/mac)
target_include_directories(loramac PUBLIC LoRaMac-node/src/boards)
target_include_directories(loramac PUBLIC LoRaMac-node/src/system)
target_include_directories(loramac PUBLIC LoRaMac-node/src/radio)
target_include_directories(loramac PUBLIC LoRaMac-node/src/peripherals/soft-se/)
target_include_directories(loramac PUBLIC LoRaMac-node/src/mac/region)

FILE(GLOB lora_sources LoRaMac-node/src/mac/*.c)
target_sources(loramac PRIVATE ${lora_sources})
FILE(GLOB lora_sources LoRaMac-node/src/peripherals/soft-se/*.c)
target_sources(loramac PRIVATE ${lora_sources})

target_sources(loramac PRIVATE LoRaMac-node/src/mac/region/RegionBaseUS.c)
target_sources(loramac PRIVATE LoRaMac-node/src/mac/region/RegionCommon.c)
target_sources(loramac PRIVATE LoRaMac-node/src/mac/region/Region.c)
# target_sources(loramac PRIVATE LoRaMac-node/src/mac/region/RegionCN470.c)
target_sources(loramac PRIVATE LoRaMac-node/src/mac/region/RegionEU433.c)

add_compile_definitions(REGION_EU433 SOFT_SE)

target_sources(loramac PRIVATE lora.c)
target_include_directories(loramac PUBLIC ./)