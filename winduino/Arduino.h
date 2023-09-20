#pragma once
#define ARDUINO
#define WINDUINO
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#define SERIAL  0x0
#define DISPLAY 0x1

#define LSBFIRST 0
#define MSBFIRST 1

//Interrupt Modes
#define RISING    0x01
#define FALLING   0x02
#define CHANGE    0x03
#define ONLOW     0x04
#define ONHIGH    0x05
#define ONLOW_WE  0x0C
#define ONHIGH_WE 0x0D

#define DEFAULT 1
#define EXTERNAL 0

#define PI 3.1415926535897932384626433832795
#define HALF_PI 1.5707963267948966192313216916398
#define TWO_PI 6.283185307179586476925286766559
#define DEG_TO_RAD 0.017453292519943295769236907684886
#define RAD_TO_DEG 57.295779513082320876798154814105
#define EULER 2.718281828459045235360287471352

#ifndef __STRINGIFY
#define __STRINGIFY(a) #a
#endif

#define _min(a,b) ((a)<(b)?(a):(b))  
#define _max(a,b) ((a)>(b)?(a):(b))
#define _abs(x) ((x)>0?(x):-(x))
#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))
#define _round(x)     ((x)>=0?(long)((x)+0.5):(long)((x)-0.5)) 
#define radians(deg) ((deg)*DEG_TO_RAD)
#define degrees(rad) ((rad)*RAD_TO_DEG)
#define sq(x) ((x)*(x))

#define interrupts() 
#define noInterrupts() 

#ifndef _NOP
#define _NOP()
#endif

#define bit(b) (1UL << (b))
#define _BV(b) (1UL << (b))

#define NOT_A_PIN -1
#define NOT_A_PORT -1
#define NOT_AN_INTERRUPT -1
#define NOT_ON_TIMER 0

//typedef bool boolean;
typedef uint8_t byte;
typedef unsigned int word;

#define LOW               0x00
#define HIGH              0x01

//GPIO FUNCTIONS
#define INPUT              0x01
// Changed OUTPUT from 0x02 to behave the same as Arduino pinMode(pin,OUTPUT) 
// where you can read the state of pin even when it is set as OUTPUT
#define OUTPUT            0x03 
#define PULLUP            0x04
#define INPUT_PULLUP      0x05
#define PULLDOWN          0x08
#define INPUT_PULLDOWN    0x09
#define OPEN_DRAIN        0x10
#define OUTPUT_OPEN_DRAIN 0x12
#define ANALOG            0xC0

//Interrupt Modes
#define DISABLED  0x00
#define RISING    0x01
#define FALLING   0x02
#define CHANGE    0x03
#define ONLOW     0x04
#define ONHIGH    0x05
#define ONLOW_WE  0x0C
#define ONHIGH_WE 0x0D

static const uint8_t A0 = 1;
static const uint8_t A1 = 2;
static const uint8_t A2 = 3;
static const uint8_t A3 = 4;
static const uint8_t A4 = 5;
static const uint8_t A5 = 6;
static const uint8_t A8 = 7;
static const uint8_t A9 = 8;
static const uint8_t A10 = 9;

static const uint8_t D0 = 1;
static const uint8_t D1 = 2;
static const uint8_t D2 = 3;
static const uint8_t D3 = 4;
static const uint8_t D4 = 5;
static const uint8_t D5 = 6;
static const uint8_t D6 = 7;
static const uint8_t D7 = 8;
static const uint8_t D8 = 9;
static const uint8_t D9 = 10;
static const uint8_t D10 = 11;

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

void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t val);
int digitalRead(uint8_t pin);

void attachInterrupt(uint8_t pin, void (*)(void), int mode);
void detachInterrupt(uint8_t pin);

const char * pathToFileName(const char * path);
void append_log_window(const char* text);
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

#include <algorithm>
#include <cmath>

#include "WCharacter.h"
#include "WString.h"
#include "Stream.h"
#include "Printable.h"
#include "Print.h"
#include "HardwareSerial.h"
