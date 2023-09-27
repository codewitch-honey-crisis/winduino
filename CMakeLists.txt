cmake_minimum_required(VERSION 3.24)
# just quick and dirty so do it this way
project(winduino VERSION 1.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(FIND_LIBRARY_USE_LIB64_PATHS True)
set( DXLIBS d2d1 )
set(CMAKE_STATIC_LIBRARY_PREFIX "")
set(CMAKE_SHARED_LIBRARY_PREFIX "")

add_subdirectory(lib/htcw_winduino)

add_library(winduino_spi_screen SHARED
    winduino_hardware/spi_screen/spi_screen.cpp)
target_link_libraries(winduino_spi_screen ${DXLIBS} )
target_include_directories(winduino_spi_screen PUBLIC
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/include"
    "${PROJECT_BINARY_DIR}"
    )

add_library(htcw_bits INTERFACE)
target_include_directories(htcw_bits INTERFACE
    "${PROJECT_SOURCE_DIR}/lib/htcw_bits/include"
)

add_library(htcw_io lib/htcw_io/src/io_stream.cpp)
target_link_libraries(htcw_io htcw_bits)
target_include_directories(htcw_io PUBLIC
"${PROJECT_SOURCE_DIR}/lib/htcw_io/include"
"${PROJECT_BINARY_DIR}")

add_library(htcw_gfx lib/htcw_gfx/src/gfx_drawing.cpp
                    lib/htcw_gfx/src/gfx_encoding.cpp
                    lib/htcw_gfx/src/gfx_font.cpp
                    lib/htcw_gfx/src/gfx_image.cpp
                    lib/htcw_gfx/src/gfx_math.cpp
                    lib/htcw_gfx/src/gfx_open_font.cpp
                    lib/htcw_gfx/src/gfx_palette.cpp
                    lib/htcw_gfx/src/gfx_pixel.cpp
                    lib/htcw_gfx/src/gfx_svg_doc.cpp
                    lib/htcw_gfx/src/gfx_vlw_font.cpp
                    lib/htcw_gfx/src/miniz.c
                    lib/htcw_gfx/src/pngle.c
)
target_link_libraries(htcw_gfx htcw_io)
target_include_directories(htcw_gfx PUBLIC
"${PROJECT_SOURCE_DIR}/lib/htcw_bits/include"
"${PROJECT_SOURCE_DIR}/lib/htcw_data/include"
"${PROJECT_SOURCE_DIR}/lib/htcw_io/include"
"${PROJECT_SOURCE_DIR}/lib/htcw_ml/include"
"${PROJECT_SOURCE_DIR}/lib/htcw_gfx/include"
"${PROJECT_BINARY_DIR}")

add_library(htcw_uix INTERFACE)
target_include_directories(htcw_uix INTERFACE
    "${PROJECT_SOURCE_DIR}/lib/htcw_uix/include"
)

add_library(htcw_tft_io INTERFACE)
target_include_directories(htcw_tft_io INTERFACE
    "${PROJECT_SOURCE_DIR}/lib/htcw_tft_io/include"
)

add_library(htcw_st7789 INTERFACE)
target_include_directories(htcw_st7789 INTERFACE
    "${PROJECT_SOURCE_DIR}/lib/htcw_st7789/include"
)

add_library(htcw_ft6236 INTERFACE)
target_include_directories(htcw_ft6236 INTERFACE
    "${PROJECT_SOURCE_DIR}/lib/htcw_ft6236/include"
)

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
