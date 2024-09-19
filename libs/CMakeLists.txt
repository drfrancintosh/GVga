# Note we are using INTERFACE so that the library can be configured per-app
# with compile-time defines

add_library(libgvga INTERFACE)

target_sources(libgvga INTERFACE
${CMAKE_CURRENT_LIST_DIR}/gfx/gfx.c
${CMAKE_CURRENT_LIST_DIR}/gvga/gvga.c
)

include_directories(
	${CMAKE_CURRENT_LIST_DIR}/gvga
	${CMAKE_CURRENT_LIST_DIR}/gfx
)

target_include_directories(libgvga INTERFACE 
${CMAKE_CURRENT_LIST_DIR}/gvga
${CMAKE_CURRENT_LIST_DIR}/gfx
)

target_compile_definitions(libgvga INTERFACE
PICO_SCANVIDEO_MAX_SCANLINE_BUFFER_WORDS=700
PICO_SCANVIDEO_SCANLINE_BUFFER_COUNT=16
)

target_link_libraries(libgvga INTERFACE
)
