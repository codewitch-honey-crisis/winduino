/*
  TwoWire.cpp - TWI/I2C library for Arduino & Wiring
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

  Modified 2012 by Todd Krein (todd@krein.org) to implement repeated starts
  Modified December 2014 by Ivan Grokhotkov (ivan@esp8266.com) - esp8266 support
  Modified April 2015 by Hrsto Gochkov (ficeto@ficeto.com) - alternative esp8266 support
  Modified Nov 2017 by Chuck Todd (ctodd@cableone.net) - ESP32 ISR Support
  Modified Nov 2021 by Hristo Gochkov <Me-No-Dev> to support ESP-IDF API
 */

extern "C" {
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
}

#include "Wire.h"
#include "Arduino.h"

TwoWire::TwoWire(uint8_t bus_num)
    :num(bus_num >=I2C_PORT_MAX?I2C_PORT_MAX-1:bus_num)
    ,bufferSize(I2C_BUFFER_LENGTH) // default Wire Buffer Size
    ,rxBuffer(NULL)
    ,rxIndex(0)
    ,rxLength(0)
    ,txBuffer(NULL)
    ,txLength(0)
    ,txAddress(0)
    ,_timeOutMillis(50)
{}

TwoWire::~TwoWire()
{
    end();
}

bool TwoWire::initPins(int sdaPin, int sclPin)
{
    if(sdaPin < 0) { // default param passed
        if(num == 0) {
            if(sda==-1) {
                sdaPin = SDA;    //use Default Pin
            } else {
                sdaPin = sda;    // reuse prior pin
            }
        } else {
            if(sda==-1) {
#ifdef WIRE1_PIN_DEFINED
                sdaPin = SDA1;
#else
                return false; //no Default pin for Second Peripheral
#endif
            } else {
                sdaPin = sda;    // reuse prior pin
            }
        }
    }

    if(sclPin < 0) { // default param passed
        if(num == 0) {
            if(scl == -1) {
                sclPin = SCL;    // use Default pin
            } else {
                sclPin = scl;    // reuse prior pin
            }
        } else {
            if(scl == -1) {
#ifdef WIRE1_PIN_DEFINED
                sclPin = SCL1;
#else
                return false; //no Default pin for Second Peripheral
#endif
            } else {
                sclPin = scl;    // reuse prior pin
            }
        }
    }

    sda = sdaPin;
    scl = sclPin;
    return true;
}

bool TwoWire::setPins(int sdaPin, int sclPin)
{
    return initPins(sdaPin,sclPin);
}

bool TwoWire::allocateWireBuffer(void)
{
    // or both buffer can be allocated or none will be
    if (rxBuffer == NULL) {
            rxBuffer = (uint8_t *)malloc(bufferSize);
            if (rxBuffer == NULL) {
                return false;
            }
    }
    if (txBuffer == NULL) {
            txBuffer = (uint8_t *)malloc(bufferSize+1);
            if (txBuffer == NULL) {
                freeWireBuffer();  // free rxBuffer for safety!
                return false;
            }
            ++txBuffer;
    }
    // in case both were allocated before, they must have the same size. All good.
    return true;
}

void TwoWire::freeWireBuffer(void)
{
    if (rxBuffer != NULL) {
        free(rxBuffer);
        rxBuffer = NULL;
    }
    if (txBuffer != NULL) {
        free(--txBuffer);
        txBuffer = NULL;
    }
}

size_t TwoWire::setBufferSize(size_t bSize)
{
    // Maximum size .... HEAP limited ;-)
    if (bSize < 32) {    // 32 bytes is the I2C FIFO Len for ESP32/S2/S3/C3
        return 0;
    }

    // allocateWireBuffer allocates memory for both pointers or just free them
    if (rxBuffer != NULL || txBuffer != NULL) {
        // if begin() has been already executed, memory size changes... data may be lost. We don't care! :^)
        if (bSize != bufferSize) {
            // we want a new buffer size ... just reset buffer pointers and allocate new ones
            freeWireBuffer();
            bufferSize = bSize;
            if (!allocateWireBuffer()) {
                // failed! Error message already issued
                bSize = 0; // returns error
            }
        } // else nothing changes, all set!
    } else {
        // no memory allocated yet, just change the size value - allocation in begin()
        bufferSize = bSize;
    }
    return bSize;
}

// Master Begin
bool TwoWire::begin(int sdaPin, int sclPin, uint32_t frequency)
{
    bool started = false;
  
    if (!allocateWireBuffer()) {
        // failed! Error Message already issued
        goto end;
    }
    if(!initPins(sdaPin, sclPin)){
        goto end;
    }
    started = true;

end:
    if (!started) freeWireBuffer();
    return started;
}

bool TwoWire::end()
{
    freeWireBuffer();
    return true;
}

uint32_t TwoWire::getClock()
{
    return this->_frequency;
}

bool TwoWire::setClock(uint32_t frequency)
{
    if(frequency==0) {
        return false;
    }
    this->_frequency = frequency;
    return true;
}
void TwoWire::setTimeOut(uint16_t timeOutMillis)
{
    _timeOutMillis = timeOutMillis;
}

uint16_t TwoWire::getTimeOut()
{
    return _timeOutMillis;
}

void TwoWire::beginTransmission(uint16_t address)
{
    nonStop = false;
    txAddress = address;
    txLength = 0;
}

/*
https://www.arduino.cc/reference/en/language/functions/communication/wire/endtransmission/
endTransmission() returns:
0: success.
1: data too long to fit in transmit buffer.
2: received NACK on transmit of address.
3: received NACK on transmit of data.
4: other error.
5: timeout
*/
uint8_t TwoWire::endTransmission(bool sendStop)
{
    if (txBuffer == NULL){
        return 4;
    }
    
    if(sendStop){
        uint8_t *p = txBuffer-1;
        *p=txAddress;
        rxLength = bufferSize;
        hardware_transfer_bytes_i2c(num,p,txLength+1,rxBuffer,&rxLength);
        rxLength = 0;
    } else {
        //mark as non-stop
        nonStop = true;
    }
    return 0;
}

size_t TwoWire::requestFrom(uint16_t address, size_t size, bool sendStop)
{
    if (rxBuffer == NULL || txBuffer == NULL){
        return 0;
    }
    if(nonStop
    ){
        if(address != txAddress){
            return 0;
        }
        nonStop = false;
        rxIndex = 0;
        rxLength = 0;
        txLength = 0;
        uint8_t *p = txBuffer-1;
        *p=txAddress|0x80;
        rxLength = bufferSize;
        hardware_transfer_bytes_i2c(num,p,txLength+1,rxBuffer,&rxLength);
    } else {
        rxIndex = 0;
        rxLength = 0;
        txLength = 0;
        uint8_t *p = txBuffer-1;
        *p=txAddress|0x80;
        rxLength = bufferSize;
        //Serial.printf("read txLength: %d\r\n",txLength);
        
        hardware_transfer_bytes_i2c(num,p,txLength+1,rxBuffer,&rxLength);
        
    }
    return rxLength;
}

size_t TwoWire::write(uint8_t data)
{
    if (txBuffer == NULL){
        return 0;
    }
    if(txLength >= bufferSize) {
        return 0;
    }
    txBuffer[txLength++] = data;
    return 1;
}

size_t TwoWire::write(const uint8_t *data, size_t quantity)
{
    for(size_t i = 0; i < quantity; ++i) {
        if(!write(data[i])) {
            return i;
        }
    }
    return quantity;

}

int TwoWire::available(void)
{
    int result = rxLength - rxIndex;
    return result;
}

int TwoWire::read(void)
{
    int value = -1;
    if (rxBuffer == NULL){
        return value;
    }
    if(rxIndex < rxLength) {
        value = rxBuffer[rxIndex++];
    }
    //Serial.printf("read() = %d\r\n",value);
    return value;
}

int TwoWire::peek(void)
{
    int value = -1;
    if (rxBuffer == NULL){
        return value;
    }
    if(rxIndex < rxLength) {
        value = rxBuffer[rxIndex];
    }
    return value;
}

void TwoWire::flush(void)
{
    rxIndex = 0;
    rxLength = 0;
    txLength = 0;
    //i2cFlush(num); // cleanup
}

size_t TwoWire::requestFrom(uint8_t address, size_t len, bool sendStop)
{
    return requestFrom(static_cast<uint16_t>(address), static_cast<size_t>(len), static_cast<bool>(sendStop));
}
  
uint8_t TwoWire::requestFrom(uint8_t address, uint8_t len, uint8_t sendStop)
{
    return requestFrom(static_cast<uint16_t>(address), static_cast<size_t>(len), static_cast<bool>(sendStop));
}

uint8_t TwoWire::requestFrom(uint16_t address, uint8_t len, uint8_t sendStop)
{
    return requestFrom(address, static_cast<size_t>(len), static_cast<bool>(sendStop));
}

/* Added to match the Arduino function definition: https://github.com/arduino/ArduinoCore-API/blob/173e8eadced2ad32eeb93bcbd5c49f8d6a055ea6/api/HardwareI2C.h#L39
 * See: https://github.com/arduino-libraries/ArduinoECCX08/issues/25
*/
uint8_t TwoWire::requestFrom(uint16_t address, uint8_t len, bool stopBit)
{
    return requestFrom((uint16_t)address, (size_t)len, stopBit);
}

uint8_t TwoWire::requestFrom(uint8_t address, uint8_t len)
{
    return requestFrom(static_cast<uint16_t>(address), static_cast<size_t>(len), true);
}

uint8_t TwoWire::requestFrom(uint16_t address, uint8_t len)
{
    return requestFrom(address, static_cast<size_t>(len), true);
}

uint8_t TwoWire::requestFrom(int address, int len)
{
    return requestFrom(static_cast<uint16_t>(address), static_cast<size_t>(len), true);
}

uint8_t TwoWire::requestFrom(int address, int len, int sendStop)
{
    return static_cast<uint8_t>(requestFrom(static_cast<uint16_t>(address), static_cast<size_t>(len), static_cast<bool>(sendStop)));
}

void TwoWire::beginTransmission(int address)
{
    beginTransmission(static_cast<uint16_t>(address));
}

void TwoWire::beginTransmission(uint8_t address)
{
    beginTransmission(static_cast<uint16_t>(address));
}

uint8_t TwoWire::endTransmission(void)
{
    return endTransmission(true);
}

#if I2C_PORT_MAX > 0
TwoWire Wire = TwoWire(0);
#endif
#if I2C_PORT_MAX > 1
TwoWire Wire1 = TwoWire(1);
#endif
#if I2C_PORT_MAX > 2
TwoWire Wire2 = TwoWire(2);
#endif
#if I2C_PORT_MAX > 3
TwoWire Wire3 = TwoWire(3);
#endif
