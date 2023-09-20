#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "Arduino.h"
#ifndef ARDUINO_SERIAL_EVENT_TASK_STACK_SIZE
#define ARDUINO_SERIAL_EVENT_TASK_STACK_SIZE 2048
#endif

#ifndef ARDUINO_SERIAL_EVENT_TASK_PRIORITY
#define ARDUINO_SERIAL_EVENT_TASK_PRIORITY (configMAX_PRIORITIES-1)
#endif

#ifndef ARDUINO_SERIAL_EVENT_TASK_RUNNING_CORE
#define ARDUINO_SERIAL_EVENT_TASK_RUNNING_CORE -1
#endif

void serialEvent(void) __attribute__((weak));
void serialEvent(void) {}

#if SOC_UART_NUM > 1

void serialEvent1(void) __attribute__((weak));
void serialEvent1(void) {}
#endif /* SOC_UART_NUM > 1 */

#if SOC_UART_NUM > 2

void serialEvent2(void) __attribute__((weak));
void serialEvent2(void) {}
#endif /* SOC_UART_NUM > 2 */

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_SERIAL)
HardwareSerial Serial(0);
#if SOC_UART_NUM > 1
HardwareSerial Serial1(1);
#endif
#if SOC_UART_NUM > 2
HardwareSerial Serial2(2);
#endif

void serialEventRun(void)
{
#if ARDUINO_USB_CDC_ON_BOOT //Serial used for USB CDC
    if(Serial0.available()) serialEvent();
#else
    if(Serial.available()) serialEvent();
#endif
#if SOC_UART_NUM > 1
    if(Serial1.available()) serialEvent1();
#endif
#if SOC_UART_NUM > 2
    if(Serial2.available()) serialEvent2();
#endif
}
#endif

#if !CONFIG_DISABLE_HAL_LOCKS
#define HSERIAL_MUTEX_LOCK()    do {} while (xSemaphoreTake(_lock, portMAX_DELAY) != pdPASS)
#define HSERIAL_MUTEX_UNLOCK()  xSemaphoreGive(_lock)
#else
#define HSERIAL_MUTEX_LOCK()    
#define HSERIAL_MUTEX_UNLOCK()  
#endif

HardwareSerial::HardwareSerial(int uart_nr) : 
_uart_nr(uart_nr), 
_rxBufferSize(256),
_txBufferSize(0), 
_onReceiveCB(NULL), 
_onReceiveErrorCB(NULL),
_onReceiveTimeout(false),
_rxTimeout(2),
_rxFIFOFull(0)
,_rxPin(-1) 
,_txPin(-1)
,_ctsPin(-1)
,_rtsPin(-1)
{
}

HardwareSerial::~HardwareSerial()
{
    end();
}


void HardwareSerial::onReceiveError(OnReceiveErrorCb function) 
{
    // function may be NULL to cancel onReceive() from its respective task 
    _onReceiveErrorCB = function;
}

void HardwareSerial::onReceive(OnReceiveCb function, bool onlyOnTimeout)
{
    _onReceiveCB = function;

    // setting the callback to NULL will just disable it
    if (_onReceiveCB != NULL) {
        // When Rx timeout is Zero (disabled), there is only one possible option that is callback when FIFO reaches 120 bytes
        _onReceiveTimeout = _rxTimeout > 0 ? onlyOnTimeout : false;

    }
    
}

// This function allow the user to define how many bytes will trigger an Interrupt that will copy RX FIFO to the internal RX Ringbuffer
// ISR will also move data from FIFO to RX Ringbuffer after a RX Timeout defined in HardwareSerial::setRxTimeout(uint8_t symbols_timeout)
// A low value of FIFO Full bytes will consume more CPU time within the ISR
// A high value of FIFO Full bytes will make the application wait longer to have byte available for the Stkech in a streaming scenario
// Both RX FIFO Full and RX Timeout may affect when onReceive() will be called
void HardwareSerial::setRxFIFOFull(uint8_t fifoBytes)
{
     // in case that onReceive() shall work only with RX Timeout, FIFO shall be high
    // this is a work around for an IDF issue with events and low FIFO Full value (< 3)
    if (_onReceiveCB != NULL && _onReceiveTimeout) {
        fifoBytes = 120;
    }
    _rxFIFOFull = fifoBytes;
}

// timout is calculates in time to receive UART symbols at the UART baudrate.
// the estimation is about 11 bits per symbol (SERIAL_8N1)
void HardwareSerial::setRxTimeout(uint8_t symbols_timeout)
{
    
    // Zero disables timeout, thus, onReceive callback will only be called when RX FIFO reaches 120 bytes
    // Any non-zero value will activate onReceive callback based on UART baudrate with about 11 bits per symbol 
    _rxTimeout = symbols_timeout;   
    if (!symbols_timeout) _onReceiveTimeout = false;  // only when RX timeout is disabled, we also must disable this flag 

}

void HardwareSerial::eventQueueReset()
{
}

void HardwareSerial::begin(unsigned long baud, uint32_t config, int8_t rxPin, int8_t txPin, bool invert, unsigned long timeout_ms, uint8_t rxfifo_full_thrhd)
{


    // Set UART FIFO Full depending on the baud rate. 
    // Lower baud rates will force to emulate byte-by-byte reading
    // Higher baud rates will keep IDF default of 120 bytes for FIFO FULL Interrupt
    // It can also be changed by the application at any time 
    if (!_rxFIFOFull) {    // it has not being changed before calling begin()
      //  set a default FIFO Full value for the IDF driver
      uint8_t fifoFull = 1;
      if (baud > 57600 || (_onReceiveCB != NULL && _onReceiveTimeout)) {
        fifoFull = 120;
      }
      _rxFIFOFull = fifoFull;
    }

    _rxPin = rxPin;
    _txPin = txPin;

}

void HardwareSerial::updateBaudRate(unsigned long baud)
{
}

void HardwareSerial::end(bool fullyTerminate)
{
    // default Serial.end() will completely disable HardwareSerial, 
    // including any tasks or debug message channel (log_x()) - but not for IDF log messages!
    if(fullyTerminate) {
        _onReceiveCB = NULL;
        _onReceiveErrorCB = NULL;
        _rxFIFOFull = 0; 

        _rxPin = _txPin = _ctsPin = _rtsPin = -1;

    }

}

void HardwareSerial::setDebugOutput(bool en)
{
}

int HardwareSerial::available(void)
{
    return 0;
}
int HardwareSerial::availableForWrite(void)
{
    return _txBufferSize;
}

int HardwareSerial::peek(void)
{
    
    return -1;
}

int HardwareSerial::read(void)
{
    
    return -1;
    
}

// read characters into buffer
// terminates if size characters have been read, or no further are pending
// returns the number of characters placed in the buffer
// the buffer is NOT null terminated.
size_t HardwareSerial::read(uint8_t *buffer, size_t size)
{
    return 0;
}

// Overrides Stream::readBytes() to be faster using IDF
size_t HardwareSerial::readBytes(uint8_t *buffer, size_t length)
{
    return 0;
}

void HardwareSerial::flush(void)
{
    
}

void HardwareSerial::flush(bool txOnly)
{
    
}

size_t HardwareSerial::write(uint8_t c)
{
    char sz[2];
    sz[1]=0;
    sz[0]=(char)c;
    append_log_window(sz);
    return 1;
}

size_t HardwareSerial::write(const uint8_t *buffer, size_t size)
{
    char* buf = (char*)malloc(size+1);
    if(buf==NULL) return 0;
    memcpy(buf,buffer,size);
    buf[size]=0;
    append_log_window(buf);
    free(buf);
    return size;
}
uint32_t  HardwareSerial::baudRate()

{
	return 115200;
}
HardwareSerial::operator bool() const
{
    return true;
}

void HardwareSerial::setRxInvert(bool invert)
{
    
}

// negative Pin value will keep it unmodified
void HardwareSerial::setPins(int8_t rxPin, int8_t txPin, int8_t ctsPin, int8_t rtsPin)
{
    
}

// Enables or disables Hardware Flow Control using RTS and/or CTS pins (must use setAllPins() before)
void HardwareSerial::setHwFlowCtrlMode(uint8_t mode, uint8_t threshold)
{
    
}

size_t HardwareSerial::setRxBufferSize(size_t new_size) {

    _rxBufferSize = new_size;
    return _rxBufferSize;
}

size_t HardwareSerial::setTxBufferSize(size_t new_size) {

 
    _txBufferSize = new_size;
    return _txBufferSize;
}