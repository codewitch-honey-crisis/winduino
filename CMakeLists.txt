cmake_minimum_required(VERSION 3.24)
project(winduino VERSION 1.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(FIND_LIBRARY_USE_LIB64_PATHS True)
set( LIBS d2d1 )
# just quick and dirty so do it this way
add_executable(winduino 
                src/main.cpp 
                
                winduino/Arduino.cpp
                winduino/WString.cpp
                winduino/stdlib_noniso.c
                winduino/Print.cpp
                winduino/Stream.cpp
                winduino/StreamString.cpp
                winduino/WMath.cpp
                winduino/StdioFSImpl.cpp
                winduino/FS.cpp

                lib/htcw_io/src/io_stream.cpp
                lib/htcw_gfx/src/gfx_drawing.cpp
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
                lib/htcw_gfx/src/pngle.c)
target_link_libraries(winduino ${LIBS} )
target_include_directories(winduino PUBLIC
                          "${PROJECT_SOURCE_DIR}/winduino"

                          "${PROJECT_SOURCE_DIR}/lib/htcw_bits/include"
                          "${PROJECT_SOURCE_DIR}/lib/htcw_io/include"
                          "${PROJECT_SOURCE_DIR}/lib/htcw_data/include"
                          "${PROJECT_SOURCE_DIR}/lib/htcw_ml/include"
                          "${PROJECT_SOURCE_DIR}/lib/htcw_gfx/include"
                          "${PROJECT_SOURCE_DIR}/lib/htcw_uix/include"

                          "${PROJECT_BINARY_DIR}"
                          "${PROJECT_SOURCE_DIR}/include"
                          )