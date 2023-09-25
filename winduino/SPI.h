/* 
  SPI.h - SPI library for esp32

  Copyright (c) 2015 Hristo Gochkov. All rights reserved.
  This file is part of the esp8266 core for Arduino environment.
 
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
*/
#ifndef _SPI_H_INCLUDED
#define _SPI_H_INCLUDED
#include "Arduino.h"
#include <stdlib.h>
#include <stdint.h>
#define SPI_HAS_TRANSACTION

// the Defines match to an AVR Arduino on 16MHz for better compatibility
#define SPI_CLOCK_DIV2    0x00101001 //8 MHz
#define SPI_CLOCK_DIV4    0x00241001 //4 MHz
#define SPI_CLOCK_DIV8    0x004c1001 //2 MHz
#define SPI_CLOCK_DIV16   0x009c1001 //1 MHz
#define SPI_CLOCK_DIV32   0x013c1001 //500 KHz
#define SPI_CLOCK_DIV64   0x027c1001 //250 KHz
#define SPI_CLOCK_DIV128  0x04fc1001 //125 KHz

#define SPI_MODE0 0
#define SPI_MODE1 1
#define SPI_MODE2 2
#define SPI_MODE3 3

#define SPI_CS0 0
#define SPI_CS1 1
#define SPI_CS2 2
#define SPI_CS_MASK_ALL 0x7

#define SPI_LSBFIRST 0
#define SPI_MSBFIRST 1


class SPISettings
{
public:
    SPISettings() :_clock(1000000), _bitOrder(SPI_MSBFIRST), _dataMode(SPI_MODE0) {}
    SPISettings(uint32_t clock, uint8_t bitOrder, uint8_t dataMode) :_clock(clock), _bitOrder(bitOrder), _dataMode(dataMode) {}
    uint32_t _clock;
    uint8_t  _bitOrder;
    uint8_t  _dataMode;
};

class SPIClass
{
private:
    uint8_t _port;
    bool _use_hw_ss;
    int16_t _sck;
    int16_t _miso;
    int16_t _mosi;
    int16_t _ss;
    uint32_t _freq;
    bool _inTransaction;
    SPISettings _transaction;
    SPISettings _restore;
    uint32_t _clock_div;
    uint8_t  _bitOrder;
    uint8_t  _dataMode;
    uint8_t _cke;
    uint8_t _ckp;
    uint8_t _delay;
public:
    SPIClass(uint8_t spi_bus=0);
    ~SPIClass();
    void begin(int16_t sck=-1, int16_t miso=-1, int16_t mosi=-1, int16_t ss=-1);
    void end();

    void setHwCs(bool use);
    void setBitOrder(uint8_t bitOrder);
    void setDataMode(uint8_t dataMode);
    void setFrequency(uint32_t freq);
    void setClockDivider(uint32_t clockDiv);
    
    uint32_t getClockDivider();

    void beginTransaction(SPISettings settings);
    void endTransaction(void);
    void transfer(void * data, uint32_t size);
    uint8_t transfer(uint8_t data);
    uint16_t transfer16(uint16_t data);
    uint32_t transfer32(uint32_t data);
  
    void transferBytes(const uint8_t * data, uint8_t * out, uint32_t size);
    void transferBits(uint32_t data, uint32_t * out, uint8_t bits);

    int8_t pinSS() { return _ss; }
};

#if SPI_PORT_MAX > 0
extern SPIClass SPI;
#endif
#if SPI_PORT_MAX > 1
extern SPIClass SPI1;
#endif
#if SPI_PORT_MAX > 2
extern SPIClass SPI2;
#endif
#if SPI_PORT_MAX > 3
extern SPIClass SPI3;
#endif
#if SPI_PORT_MAX > 4
extern SPIClass SPI4;
#endif
#if SPI_PORT_MAX > 5
extern SPIClass SPI5;
#endif

#endif