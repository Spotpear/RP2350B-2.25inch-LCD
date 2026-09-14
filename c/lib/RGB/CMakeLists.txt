# WS2812 RGB breathing-light library (single NeoPixel on GP25, driven via PIO)
add_library(RGB RGB.c)

target_include_directories(RGB PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
    ../Config
)

# Generate ws2812.pio.h from ws2812.pio into this target's build dir.
pico_generate_pio_header(RGB ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio)

target_link_libraries(RGB PUBLIC
    Config
    pico_stdlib
    hardware_pio
    hardware_clocks
    pico_multicore
)
