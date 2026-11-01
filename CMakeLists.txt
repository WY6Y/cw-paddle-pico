cmake_minimum_required(VERSION 3.13)
# pico2 (default) or pico2_w: cmake -B build-w -DPICO_BOARD=pico2_w
set(PICO_BOARD pico2 CACHE STRING "Board")
include(pico_sdk_import.cmake)
project(cw_paddle C CXX ASM)
pico_sdk_init()

add_executable(cw_paddle src/main.c src/usb_descriptors.c)
target_include_directories(cw_paddle PRIVATE src)
target_link_libraries(cw_paddle PRIVATE pico_stdlib pico_unique_id tinyusb_device hardware_watchdog)
if(PICO_BOARD STREQUAL "pico2_w")
    target_link_libraries(cw_paddle PRIVATE pico_cyw43_arch_none)
endif()
pico_add_extra_outputs(cw_paddle)
