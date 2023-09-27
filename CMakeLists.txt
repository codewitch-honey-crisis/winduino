cmake_minimum_required(VERSION 3.24)

project(example VERSION 1.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(FIND_LIBRARY_USE_LIB64_PATHS True)
set( DXLIBS d2d1 )
set(CMAKE_STATIC_LIBRARY_PREFIX "")
set(CMAKE_SHARED_LIBRARY_PREFIX "")

add_subdirectory(lib/htcw_winduino)

#uix/gfx library dependencies
add_subdirectory(lib/htcw_bits)
add_subdirectory(lib/htcw_data)
add_subdirectory(lib/htcw_io)
add_subdirectory(lib/htcw_ml)
add_subdirectory(lib/htcw_gfx)
add_subdirectory(lib/htcw_uix)

#for SPI screen testing
add_subdirectory(lib/htcw_tft_io)
add_subdirectory(lib/htcw_ft6236)
add_subdirectory(lib/htcw_st7789)

#SPI screen hardware emulation
add_subdirectory(winduino_hardware/spi_screen)

add_executable(example 
    src/main.cpp
)
target_link_libraries(example htcw_winduino htcw_gfx htcw_uix htcw_tft_io htcw_st7789 htcw_ft6236)
target_include_directories(example PUBLIC
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_BINARY_DIR}"
    "${PROJECT_SOURCE_DIR}/include"
    "${PROJECT_SOURCE_DIR}/src"
  )
