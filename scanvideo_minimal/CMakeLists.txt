cmake_minimum_required(VERSION 3.15)
set(PROJECT scanvideo_minimal)
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
include($ENV{PICO_EXTRAS_PATH}/external/pico_extras_import.cmake)

project(${PROJECT} C CXX)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the Pico SDK
pico_sdk_init()

if (PICO_SDK_VERSION_STRING VERSION_LESS "2.0.0")
    message(FATAL_ERROR "Require at least Raspberry Pi Pico SDK version 2.0.0")
endif()

add_executable(scanvideo_minimal
        scanvideo_minimal.c
)

pico_enable_stdio_usb(${PROJECT} 1)
pico_enable_stdio_uart(${PROJECT} 0)

target_link_libraries(${PROJECT} PRIVATE
        pico_multicore
        pico_stdlib
        pico_scanvideo_dpi
)

pico_add_extra_outputs(${PROJECT})
