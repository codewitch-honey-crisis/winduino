#pragma once
#include <stdint.h>
// useful for libs like LVGL that can't use DirectX native format
// #define USE_RGBA8888
constexpr static const struct {
    int width;
    int height;
} screen_size = 
#ifdef SCREEN_SIZE
SCREEN_SIZE;
#else
{320,240};
#endif
/// @brief Reports the milliseconds since the app started
/// @return The number of milliseconds elapsed
uint32_t millis();
/// @brief Delays for the number of milliseconds
/// @return The number of milliseconds to delay
void delay(uint32_t ms);

/// @brief The start routine - similar to main()
void setup();
/// @brief The loop() routine, as in Arduino
void loop();
/// @brief Flushes a bitmap to the display
/// @param x1 The left x coordinate
/// @param y1 The top y coordinate
/// @param w The width
/// @param h The height
/// @param bmp The bitmap in DirectX pixel format
void flush_bitmap(int x1, int y1, int w, int h, const void* bmp );
/// @brief Reads the mouse information
/// @param out_location The location
/// @return True if the button is pressed
bool read_mouse(int* out_x, int* out_y);

/// @brief Emulates a subset of the Arduino serial to ouput to the log window
class HardwareSerial {
public:
    /// @brief Does nothing
    /// @param baud not relevant
    /// @param ignored not relevant
    /// @param ignored2 not relevant
    /// @param ignored3 not relevant
    void begin(int baud,int ignored=0,int ignored2=0,int ignored3=0);
    /// @brief Prints some text to the log
    /// @param text The text
    void print(const char* text);
    /// @brief Prints text followed by a newline to the log
    /// @param text The text
    void println(const char* text);
    /// @brief Prints formatted text to the log
    /// @param fmt The format string
    /// @param  The arguments
    void printf(const char* fmt,...);
};
/// @brief The primarily serial
extern HardwareSerial Serial;
/// @brief For compatibility
extern HardwareSerial USBSerial;