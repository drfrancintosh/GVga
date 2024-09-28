cmake_minimum_required(VERSION 3.15)
cmake_policy(SET CMP0079 NEW)
set(FAMILY rp2040)
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
include($ENV{PICO_EXTRAS_PATH}/external/pico_extras_import.cmake)

set(PROJECT text_mode)
project(${PROJECT})
pico_sdk_init()

add_compile_options(-Wall)
add_executable(${PROJECT})

include_directories(
	$ENV{PICO_DVI_HOME}/include
)

target_sources(${PROJECT} PUBLIC
    src/main.c
)

# Example include
target_include_directories(${PROJECT} PUBLIC
    src
)

target_link_libraries(${PROJECT} PUBLIC 
    pico_stdlib
    pico_multicore
    pico_unique_id
    hardware_i2c
    libgvga
)


target_compile_definitions(${PROJECT} PRIVATE
PICO_SCANVIDEO_MAX_SCANLINE_BUFFER_WORDS=700
)

add_subdirectory($ENV{GVGA_HOME}/libs ${CMAKE_BINARY_DIR}/libgvga)

target_link_libraries(${PROJECT} PRIVATE
        pico_multicore
        pico_stdlib
        pico_scanvideo_dpi
        libgvga
)

pico_add_extra_outputs(${PROJECT})
