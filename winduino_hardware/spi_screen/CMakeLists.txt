cmake_minimum_required(VERSION 3.24)
project(winduino_spi_screen VERSION 1.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(FIND_LIBRARY_USE_LIB64_PATHS True)
set( LIBS d2d1 htcw_winduino)
set(CMAKE_STATIC_LIBRARY_PREFIX "")
set(CMAKE_SHARED_LIBRARY_PREFIX "")

add_library(winduino_spi_screen SHARED
    spi_screen.cpp)
target_link_libraries(winduino_spi_screen ${LIBS} )
target_include_directories(winduino_spi_screen PUBLIC
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/include"
    "${PROJECT_BINARY_DIR}"
    )

target_link_libraries(winduino_spi_screen ${LIBS} )
target_include_directories(winduino_spi_screen PRIVATE
    "${PROJECT_SOURCE_DIR}"    
    "${PROJECT_BINARY_DIR}"
)

