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
#ifndef I2C_PORT_MAX
#define I2C_PORT_MAX 4
#endif
#ifndef SPI_PORT_MAX
#define SPI_PORT_MAX 6
#endif
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

#define digitalPinToGPIONumber(digitalPin) (digitalPin)
#define gpioNumberToDigitalPin(gpioNumber) (gpioNumber)
#define digitalPinToBitMask(pin)    (1UL << digitalPinToGPIONumber(pin))

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
typedef void* hw_handle_t;
// useful for libs like LVGL that can't use DirectX native format
// #define USE_RGBA8888
constexpr static const struct {
    int width;
    int height;
} winduino_screen_size = 
#ifdef SCREEN_SIZE
SCREEN_SIZE;
#else
{320,240};
#endif
typedef __cdecl void(*hardware_log_callback)(const char* text);
/// @brief Reports the milliseconds since the app started
/// @return The number of milliseconds elapsed
uint32_t millis();

/// @brief Reports the microseconds since the app started
/// @return The number of microseconds elapsed
uint32_t micros();

/// @brief Delays for the number of milliseconds
/// @return The number of milliseconds to delay
void delay(uint32_t ms);

/// @brief Delays for the number of microseconds
/// @return The number of milliseconds to delay
void delayMicroseconds(uint32_t us);

void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t val);
int digitalRead(uint8_t pin);
inline uint8_t digitalPinToInterrupt(uint8_t pin) { return pin; }
void attachInterrupt(uint8_t pin, void (*)(void), int mode);
void detachInterrupt(uint8_t pin);
void analogWrite(uint8_t pin, int value);
uint16_t analogRead(uint8_t pin);
inline void analogWriteResolution(uint8_t bits) {}
const char * pathToFileName(const char * path);
void append_log_window(const char* text);
void yield();
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
/// @brief Loads a DLL that emulates hardware 
/// @param name The DLL file to load
/// @return a handle to the loaded hardware
hw_handle_t hardware_load(const char* name);
/// @brief Attaches a pin to the hardware
/// @param hw A handle to the hardware
/// @param mcu_pin The pin on the virtual MCU
/// @param hw_pin The pin in the virtual hardare (included with header)
/// @return True if success, otherwise false
bool hardware_set_pin(hw_handle_t hw, uint8_t mcu_pin, uint8_t hw_pin);
/// @brief Configures the hardware. This must be done immediately after load
/// @param hw A handle to the hardware
/// @param prop The property to set (included with the header)
/// @param data The value (type depends on the property)
/// @param size The size of the value data
/// @return True if successful, otherwise false
bool hardware_configure(hw_handle_t hw, int prop, void* data, size_t size);
/// @brief Transmits the specified number of bits over the virtual SPI subsystem
/// @param data The data to transmit, on return this contains new data
/// @param size_bits The number of bits of data to transmit
/// @return True if successful, otherwise false
bool hardware_transfer_bits_spi(uint8_t port,uint8_t* data, size_t size_bits);
/// @brief Transmits the specified number of bytes over the virtual I2C subsystem
/// @param in The input buffer
/// @param in_size The size of the input contents
/// @param out The output buffer
/// @param in_out_out_size The size of the input buffer. On return the size of the data actually read
/// @return True if successful, otherwise false
bool hardware_transfer_bytes_i2c(uint8_t port,const uint8_t*in, size_t in_size, uint8_t* out,size_t* in_out_out_size);
/// @brief Attaches the logging system to the hardware so that it can log to the console
/// @param hw A handle to the hardware
/// @return True if successful, otherwise false
bool hardware_attach_log(hw_handle_t hw);
/// @brief Attaches hardware to an SPI port
/// @param hw The hardware handle
/// @param port The SPI port to attach to
/// @return True if successful, otherwise false
bool hardware_attach_spi(hw_handle_t hw, uint8_t port);
/// @brief Attaches hardware to an I2C port
/// @param hw The hardware handle
/// @param port The I2C port to attach to
/// @return True if successful, otherwise false
bool hardware_attach_i2c(hw_handle_t hw, uint8_t port);
#include <algorithm>
#include <cmath>
#include <math.h>
#include "WCharacter.h"
#include "WString.h"
#include "WMath.h"
#include "Stream.h"
#include "Printable.h"
#include "Print.h"
#include "HardwareSerial.h"
inline int min(int x,int y) { return x<y?x:y; }
inline int max(int x,int y) { return x>y?x:y; }
