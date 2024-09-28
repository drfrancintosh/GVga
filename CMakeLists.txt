cmake_minimum_required(VERSION 3.12)
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
set(PROJECT GVga)
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
include($ENV{PICO_EXTRAS_PATH}/external/pico_extras_import.cmake)

project(${PROJECT} C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")

pico_sdk_init()

add_compile_options(-Wall)

include_directories(
	$ENV{PICO_DVI_HOME}/software/include
)

add_subdirectory($ENV{PICO_DVI_HOME}/software/libdvi ${CMAKE_BINARY_DIR}/libdvi)
add_subdirectory(libs)
# add_subdirectory(apps)
