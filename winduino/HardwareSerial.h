/*
 HardwareSerial.h - Hardware serial library for Wiring
 Copyright (c) 2006 Nicholas Zambetti.  All right reserved.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

 Modified 28 September 2010 by Mark Sproul
 Modified 14 August 2012 by Alarus
 Modified 3 December 2013 by Matthijs Kooijman
 Modified 18 December 2014 by Ivan Grokhotkov (esp8266 platform support)
 Modified 31 March 2015 by Markus Sattler (rewrite the code for UART0 + UART1 support in ESP8266)
 Modified 25 April 2015 by Thomas Flayols (add configuration different from 8N1 in ESP8266)
 Modified 13 October 2018 by Jeroen Döll (add baudrate detection)
 Baudrate detection example usage (detection on Serial1):
   void setup() {
     Serial.begin(115200);
     delay(100);
     Serial.println();

     Serial1.begin(0, SERIAL_8N1, -1, -1, true, 11000UL);  // Passing 0 for baudrate to detect it, the last parameter is a timeout in ms

     unsigned long detectedBaudRate = Serial1.baudRate();
     if(detectedBaudRate) {
       Serial.printf("Detected baudrate is %lu\n", detectedBaudRate);
     } else {
       Serial.println("No baudrate detected, Serial1 will not work!");
     }
   }

 Pay attention: the baudrate returned by baudRate() may be rounded, eg 115200 returns 115201
 */

#ifndef HardwareSerial_h
#define HardwareSerial_h

#include <inttypes.h>
#include <functional>
#include "Stream.h"

enum SerialConfig {
SERIAL_5N1 = 0x8000010,
SERIAL_6N1 = 0x8000014,
SERIAL_7N1 = 0x8000018,
SERIAL_8N1 = 0x800001c,
SERIAL_5N2 = 0x8000030,
SERIAL_6N2 = 0x8000034,
SERIAL_7N2 = 0x8000038,
SERIAL_8N2 = 0x800003c,
SERIAL_5E1 = 0x8000012,
SERIAL_6E1 = 0x8000016,
SERIAL_7E1 = 0x800001a,
SERIAL_8E1 = 0x800001e,
SERIAL_5E2 = 0x8000032,
SERIAL_6E2 = 0x8000036,
SERIAL_7E2 = 0x800003a,
SERIAL_8E2 = 0x800003e,
SERIAL_5O1 = 0x8000013,
SERIAL_6O1 = 0x8000017,
SERIAL_7O1 = 0x800001b,
SERIAL_8O1 = 0x800001f,
SERIAL_5O2 = 0x8000033,
SERIAL_6O2 = 0x8000037,
SERIAL_7O2 = 0x800003b,
SERIAL_8O2 = 0x800003f
};
typedef enum {
    UART_NO_ERROR,
    UART_BREAK_ERROR,
    UART_BUFFER_FULL_ERROR,
    UART_FIFO_OVF_ERROR,
    UART_FRAME_ERROR,
    UART_PARITY_ERROR
} hardwareSerial_error_t;

typedef std::function<void(void)> OnReceiveCb;
typedef std::function<void(hardwareSerial_error_t)> OnReceiveErrorCb;

class HardwareSerial: public Stream
{
public:
    HardwareSerial(int uart_nr);
    ~HardwareSerial();

    // setRxTimeout sets the timeout after which onReceive callback will be called (after receiving data, it waits for this time of UART rx inactivity to call the callback fnc)
    // param symbols_timeout defines a timeout threshold in uart symbol periods. Setting 0 symbol timeout disables the callback call by timeout.
    //                       Maximum timeout setting is calculacted automatically by IDF. If set above the maximum, it is ignored and an error is printed on Serial0 (check console).
    //                       Examples: Maximum for 11 bits symbol is 92 (SERIAL_8N2, SERIAL_8E1, SERIAL_8O1, etc), Maximum for 10 bits symbol is 101 (SERIAL_8N1).
    //                       For example symbols_timeout=1 defines a timeout equal to transmission time of one symbol (~11 bit) on current baudrate. 
    //                       For a baudrate of 9600, SERIAL_8N1 (10 bit symbol) and symbols_timeout = 3, the timeout would be 3 / (9600 / 10) = 3.125 ms
    void setRxTimeout(uint8_t symbols_timeout);

    // setRxFIFOFull(uint8_t fifoBytes) will set the number of bytes that will trigger UART_INTR_RXFIFO_FULL interrupt and fill up RxRingBuffer
    // This affects some functions such as Serial::available() and Serial.read() because, in a UART flow of receiving data, Serial internal 
    // RxRingBuffer will be filled only after these number of bytes arrive or a RX Timeout happens.
    // This parameter can be set to 1 in order to receive byte by byte, but it will also consume more CPU time as the ISR will be activates often.
    void setRxFIFOFull(uint8_t fifoBytes);

    // onReceive will setup a callback that will be called whenever an UART interruption occurs (UART_INTR_RXFIFO_FULL or UART_INTR_RXFIFO_TOUT)
    // UART_INTR_RXFIFO_FULL interrupt triggers at UART_FULL_THRESH_DEFAULT bytes received (defined as 120 bytes by default in IDF)
    // UART_INTR_RXFIFO_TOUT interrupt triggers at UART_TOUT_THRESH_DEFAULT symbols passed without any reception (defined as 10 symbos by default in IDF)
    // onlyOnTimeout parameter will define how onReceive will behave:
    // Default: true -- The callback will only be called when RX Timeout happens. 
    //                  Whole stream of bytes will be ready for being read on the callback function at once.
    //                  This option may lead to Rx Overflow depending on the Rx Buffer Size and number of bytes received in the streaming
    //         false -- The callback will be called when FIFO reaches 120 bytes and also on RX Timeout.
    //                  The stream of incommig bytes will be "split" into blocks of 120 bytes on each callback.
    //                  This option avoid any sort of Rx Overflow, but leaves the UART packet reassembling work to the Application.
    void onReceive(OnReceiveCb function, bool onlyOnTimeout = false);

    // onReceive will be called on error events (see hardwareSerial_error_t)
    void onReceiveError(OnReceiveErrorCb function);

    // eventQueueReset clears all events in the queue (the events that trigger onReceive and onReceiveError) - maybe usefull in some use cases
    void eventQueueReset();
 
    void begin(unsigned long baud, uint32_t config=SERIAL_8N1, int8_t rxPin=-1, int8_t txPin=-1, bool invert=false, unsigned long timeout_ms = 20000UL, uint8_t rxfifo_full_thrhd = 112);
    void end(bool fullyTerminate = true);
    void updateBaudRate(unsigned long baud);
    int available(void);
    int availableForWrite(void);
    int peek(void);
    int read(void);
    size_t read(uint8_t *buffer, size_t size);
    inline size_t read(char * buffer, size_t size)
    {
        return read((uint8_t*) buffer, size);
    }
    // Overrides Stream::readBytes() to be faster using IDF
    size_t readBytes(uint8_t *buffer, size_t length);
    size_t readBytes(char *buffer, size_t length)
    {
        return readBytes((uint8_t *) buffer, length);
    }    
    void flush(void);
    void flush( bool txOnly);
    size_t write(uint8_t);
    size_t write(const uint8_t *buffer, size_t size);
    inline size_t write(const char * buffer, size_t size)
    {
        return write((uint8_t*) buffer, size);
    }
    inline size_t write(const char * s)
    {
        return write((uint8_t*) s, strlen(s));
    }
    inline size_t write(unsigned long n)
    {
        return write((uint8_t) n);
    }
    inline size_t write(long n)
    {
        return write((uint8_t) n);
    }
    inline size_t write(unsigned int n)
    {
        return write((uint8_t) n);
    }
    inline size_t write(int n)
    {
        return write((uint8_t) n);
    }
    uint32_t baudRate();
    operator bool() const;

    void setDebugOutput(bool);
    
    void setRxInvert(bool);

    // Negative Pin Number will keep it unmodified, thus this function can set individual pins
    // SetPins shall be called after Serial begin()
    void setPins(int8_t rxPin, int8_t txPin, int8_t ctsPin = -1, int8_t rtsPin = -1);
    // Enables or disables Hardware Flow Control using RTS and/or CTS pins (must use setAllPins() before)
    void setHwFlowCtrlMode(uint8_t mode = 0, uint8_t threshold = 64);   // 64 is half FIFO Length

    size_t setRxBufferSize(size_t new_size);
    size_t setTxBufferSize(size_t new_size);

protected:
    int _uart_nr;
    size_t _rxBufferSize;
    size_t _txBufferSize;
    OnReceiveCb _onReceiveCB;
    OnReceiveErrorCb _onReceiveErrorCB;
    // _onReceive and _rxTimeout have be consistent when timeout is disabled
    bool _onReceiveTimeout;
    uint8_t _rxTimeout, _rxFIFOFull;
    
    int8_t _rxPin, _txPin, _ctsPin, _rtsPin;

};

extern void serialEventRun(void) __attribute__((weak));

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_SERIAL)
#ifndef ARDUINO_USB_CDC_ON_BOOT
#define ARDUINO_USB_CDC_ON_BOOT 0
#endif
#if ARDUINO_USB_CDC_ON_BOOT //Serial used for USB CDC
#if !ARDUINO_USB_MODE
#include "USB.h"
#include "USBCDC.h"
#endif
extern HardwareSerial Serial0;
#else
extern HardwareSerial Serial;
#endif
#if SOC_UART_NUM > 1
extern HardwareSerial Serial1;
#endif
#if SOC_UART_NUM > 2
extern HardwareSerial Serial2;
#endif
#endif

#endif // HardwareSerial_h