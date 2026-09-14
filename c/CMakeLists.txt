# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.0)
set(toolchainVersion 13_3_Rel1)
set(picotoolVersion 2.1.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.0)
set(toolchainVersion 13_3_Rel1)
set(picotoolVersion 2.1.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
# This board is an RP2350B (48-GPIO) part. The SD card (GP32-35) and IMU
# (GP36/37) pins only exist on the "B" variant, so we must select the custom
# board header in ./boards (which sets PICO_RP2350A=0). The pin assignments
# themselves are unchanged - this only makes those high GPIOs valid.
list(APPEND PICO_BOARD_HEADER_DIRS ${CMAKE_CURRENT_LIST_DIR}/boards)
set(PICO_BOARD pico2_rp2350b CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(Pico_ePaper_Code C CXX ASM)
pico_sdk_init()

# FatFs + SD card driver (SD card on SPI0: SCK=GP34, MOSI=GP35, MISO=GP32, CS=GP33)
add_library(FatFs INTERFACE)
target_sources(FatFs INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/lib/FatFs/ff15/source/ff.c
    ${CMAKE_CURRENT_LIST_DIR}/lib/FatFs/ff15/source/ffsystem.c
    ${CMAKE_CURRENT_LIST_DIR}/lib/FatFs/ff15/source/ffunicode.c
    ${CMAKE_CURRENT_LIST_DIR}/lib/FatFs/sd_driver/dma_interrupts.c
    ${CMAKE_CURRENT_LIST_DIR}/lib/FatFs/sd_driver/sd_card.c
    ${CMAKE_CURRENT_LIST_DIR}/lib/FatFs/sd_driver/SDIO/rp2040_sdio.c
    ${CMAKE_CURRENT_LIST_DIR}/lib/FatFs/sd_driver/SDIO/sd_card_sdio.c
    ${CMAKE_CURRENT_LIST_DIR}/lib/FatFs/sd_driver/SPI/my_spi.c
    ${CMAKE_CURRENT_LIST_DIR}/lib/FatFs/sd_driver/SPI/sd_card_spi.c
    ${CMAKE_CURRENT_LIST_DIR}/lib/FatFs/sd_driver/SPI/sd_spi.c
    ${CMAKE_CURRENT_LIST_DIR}/lib/FatFs/src/crash.c
    ${CMAKE_CURRENT_LIST_DIR}/lib/FatFs/src/crc.c
    ${CMAKE_CURRENT_LIST_DIR}/lib/FatFs/src/f_util.c
    ${CMAKE_CURRENT_LIST_DIR}/lib/FatFs/src/ff_stdio.c
    ${CMAKE_CURRENT_LIST_DIR}/lib/FatFs/src/file_stream.c
    ${CMAKE_CURRENT_LIST_DIR}/lib/FatFs/src/glue.c
    ${CMAKE_CURRENT_LIST_DIR}/lib/FatFs/src/my_debug.c
    ${CMAKE_CURRENT_LIST_DIR}/lib/FatFs/src/my_rtc.c
    ${CMAKE_CURRENT_LIST_DIR}/lib/FatFs/src/sd_timeouts.c
    ${CMAKE_CURRENT_LIST_DIR}/lib/FatFs/src/util.c
    ${CMAKE_CURRENT_LIST_DIR}/lib/FatFs/config/hw_config.c
)
target_include_directories(FatFs INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/lib/FatFs/ff15/source
    ${CMAKE_CURRENT_LIST_DIR}/lib/FatFs/sd_driver
    ${CMAKE_CURRENT_LIST_DIR}/lib/FatFs/include
)
target_link_libraries(FatFs INTERFACE
    hardware_dma
    hardware_pio
    hardware_spi
    pico_aon_timer
    pico_stdlib
    cmsis_core
)

#Add compilation subdirectory
add_subdirectory(lib/Config)
add_subdirectory(lib/Infrared)

add_subdirectory(lib/Icm20948)
add_subdirectory(lib/QMI8658)
add_subdirectory(lib/LCD)
add_subdirectory(lib/Fonts)
add_subdirectory(lib/GUI)
add_subdirectory(lib/RGB)
add_subdirectory(examples)



#Add header file directory
include_directories(examples)
include_directories(./lib/Config)
include_directories(./lib/GUI)
include_directories(./lib/Icm20948)
include_directories(./lib/QMI8658)
include_directories(./lib/LCD)
include_directories(./lib/RGB)
include_directories(./lib/Infrared)
include_directories(./lib/FatFs/include)
include_directories(./lib/FatFs/sd_driver)
include_directories(./lib/FatFs/ff15/source)



# Generate executable file
add_executable(main
main.c
)

# Generate PIO header for the SDIO driver (required by the SD card driver sources)
pico_generate_pio_header(main ${CMAKE_CURRENT_LIST_DIR}/lib/FatFs/sd_driver/SDIO/rp2040_sdio.pio)

# enable usb output, disable uart output
pico_enable_stdio_usb(main 1)
pico_enable_stdio_uart(main 1)

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(main)

target_link_libraries(main examples LCD  Infrared Icm20948 QMI8658 GUI Fonts Config FatFs RGB pico_stdlib hardware_spi hardware_dma hardware_pio pico_multicore)