/* Portions from:
 * Copyright (c) 2014, Majenko Technologies
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *
 *  2. Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *
 *  3. Neither the name of Majenko Technologies nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "SPI.h"

#include "Arduino.h"

#if SPI_PORT_MAX > 0
SPIClass SPI = SPIClass(0);
#endif
#if SPI_PORT_MAX > 1
SPIClass SPI1 = SPIClass(1);
#endif
#if SPI_PORT_MAX > 2
SPIClass SPI2 = SPIClass(2);
#endif
#if SPI_PORT_MAX > 3
SPIClass SPI3 = SPIClass(3);
#endif
#if SPI_PORT_MAX > 4
SPIClass SPI4 = SPIClass(4);
#endif
#if SPI_PORT_MAX > 5
SPIClass SPI5 = SPIClass(5);
#endif


SPIClass::SPIClass(uint8_t spi_bus) : _port(spi_bus), _use_hw_ss(true), _sck(-1), _miso(-1), _mosi(-1), _ss(-1), _freq(10 * 1000 * 1000), _inTransaction(false),_clock_div(SPI_CLOCK_DIV2),_bitOrder(MSBFIRST),_dataMode(SPI_MODE0),_cke(0),_ckp(0),_delay(2) {
}
SPIClass::~SPIClass() {
    end();
}


void SPIClass::begin(int16_t sck, int16_t miso, int16_t mosi, int16_t ss) {
    if (sck == -1 || (miso == -1 && mosi == -1)) {
        return;
    }
    pinMode(sck, OUTPUT);
    digitalWrite(sck, LOW);
    _sck = sck;
    if (miso != -1) {
        pinMode(miso, INPUT);
    }
    _miso = miso;
    if (mosi != -1) {
        pinMode(mosi, OUTPUT);
        digitalWrite(mosi, LOW);
    }
    _mosi = mosi;
    if (ss != -1) {
        pinMode(ss, OUTPUT);
        digitalWrite(ss, HIGH);
        _use_hw_ss = true;
    }
    _ss = ss;
    setBitOrder(MSBFIRST);
    setDataMode(SPI_MODE0);
    setClockDivider(SPI_CLOCK_DIV2);
    _inTransaction = false;
}
void SPIClass::setHwCs(bool use) {
    _use_hw_ss = use;
}
void SPIClass::setBitOrder(uint8_t bitOrder) {
    _bitOrder = bitOrder;
}
void SPIClass::setDataMode(uint8_t dataMode) {
    _dataMode = dataMode;
    switch (dataMode) {
        case SPI_MODE0:
            _ckp = 0;
            _cke = 0;
            break;
        case SPI_MODE1:
            _ckp = 0;
            _cke = 1;
            break;
        case SPI_MODE2:
            _ckp = 1;
            _cke = 0;
            break;
        case SPI_MODE3:
            _ckp = 1;
            _cke = 1;
            break;
    }
    digitalWrite(_sck, _ckp ? HIGH : LOW);
}
void SPIClass::setFrequency(uint32_t freq) {
    _freq = freq;
}
void SPIClass::setClockDivider(uint32_t clockDiv) {
    _clock_div = clockDiv;
    switch (_clock_div) {
        case SPI_CLOCK_DIV2:
            _delay = 2;
            break;
        case SPI_CLOCK_DIV4:
            _delay = 4;
            break;
        case SPI_CLOCK_DIV8:
            _delay = 8;
            break;
        case SPI_CLOCK_DIV16:
            _delay = 16;
            break;
        case SPI_CLOCK_DIV32:
            _delay = 32;
            break;
        case SPI_CLOCK_DIV64:
            _delay = 64;
            break;
        case SPI_CLOCK_DIV128:
            _delay = 128;
            break;
        default:
            _delay = 128;
            break;
    }
}
uint32_t SPIClass::getClockDivider() {
    return _clock_div;
}

void SPIClass::beginTransaction(SPISettings settings) {
    if (_inTransaction == true) {
        return;
    }
    _transaction = settings;
    _restore._bitOrder = _bitOrder;
    _restore._clock = _freq;
    _restore._dataMode = _dataMode;
    setBitOrder(settings._bitOrder);
    setDataMode(settings._dataMode);
    setFrequency(settings._clock);
    _inTransaction = true;
    if (_ss != -1 && _use_hw_ss) {
        digitalWrite(_ss, LOW);
    }
}
void SPIClass::endTransaction(void) {
    if (_inTransaction) {
        _inTransaction = false;
        if (_ss != -1 && _use_hw_ss) {
            digitalWrite(_ss, HIGH);
        }
        setBitOrder(_restore._bitOrder);
        setDataMode(_restore._dataMode);
        setFrequency(_restore._clock);
    }
}
void SPIClass::transfer(void* data, uint32_t size) {
    bool reset_cs;
    if(reset_cs=(!_inTransaction && _use_hw_ss && _ss!=-1)) {
        digitalWrite(_ss,LOW);
        
    }
    hardware_transfer_bits_spi(_port,(uint8_t*)data,size*8);
    if(reset_cs) {
        digitalWrite(_ss,HIGH);
    }
}
uint8_t SPIClass::transfer(uint8_t val) {
    bool reset_cs;
    if(reset_cs=(!_inTransaction && _use_hw_ss && _ss!=-1)) {
        digitalWrite(_ss,LOW);
        
    }
    hardware_transfer_bits_spi(_port,&val,8);
    if(reset_cs) {
        digitalWrite(_ss,HIGH);
    }
    return val;
}
uint16_t SPIClass::transfer16(uint16_t data) {
    union {
        uint16_t val;
        struct {
            uint8_t lsb;
            uint8_t msb;
        };
    } in, out;

    in.val = data;

    if (_bitOrder == MSBFIRST) {
        out.msb = transfer(in.msb);
        out.lsb = transfer(in.lsb);
    } else {
        out.lsb = transfer(in.lsb);
        out.msb = transfer(in.msb);
    }

    return out.val;
}
uint32_t SPIClass::transfer32(uint32_t data) {
    union {
        uint32_t val;
        struct {
            uint16_t lsb;
            uint16_t msb;
        };
    } in, out;
    if (_bitOrder == MSBFIRST) {
        out.msb = transfer16(in.msb);
        out.lsb = transfer16(in.lsb);
    } else {
        out.lsb = transfer16(in.lsb);
        out.msb = transfer16(in.msb);
    }
    return out.val;
}

void SPIClass::end() {
    _inTransaction = false;
}