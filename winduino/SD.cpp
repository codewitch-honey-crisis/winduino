// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <Windows.h>
#include <fileapi.h>
#include "StdioFSImpl.h"
#include "FS.h"
#include "SD.h"

using namespace fs;

SDFS::SDFS(FSImplPtr impl): FS(impl), _pdrv(0xFF) {}

bool SDFS::begin(const char * mountpoint, uint8_t max_files, bool format_if_empty)
{
    if(_pdrv != 0xFF) {
        return true;
    }

    _impl->mountpoint(mountpoint);
    return true;
}

void SDFS::end()
{
    if(_pdrv != 0xFF) {
        _impl->mountpoint(NULL);
        _pdrv = 0xFF;
    }
}

sdcard_type_t SDFS::cardType()
{
    if(_pdrv == 0xFF) {
        return CARD_NONE;
    }
    return CARD_UNKNOWN;
}

uint64_t SDFS::cardSize()
{
    if(_pdrv == 0xFF) {
        return 0;
    }
    ULARGE_INTEGER FreeBytesAvailable = { 0 };
    ULARGE_INTEGER TotalNumberOfBytes={ 0 };
    ULARGE_INTEGER TotalNumberOfFreeBytes={ 0 };

    BOOL ok = GetDiskFreeSpaceEx(
        NULL,
        &FreeBytesAvailable,
        &TotalNumberOfBytes,
        &TotalNumberOfFreeBytes
        );

    if(!ok) {
        return 0;
    }
    return TotalNumberOfBytes.QuadPart;
    
}

size_t SDFS::numSectors()
{
    if(_pdrv == 0xFF) {
        return 0;
    }
    DWORD sectorsPerCluster;
    DWORD bytesPerSector;
    DWORD numberOfFreeClusters;
    DWORD totalNumberOfClusters;
    BOOL ok= GetDiskFreeSpaceA(NULL,&sectorsPerCluster,&bytesPerSector,&numberOfFreeClusters,&totalNumberOfClusters);
    if(!ok) {
        return 0;
    }
    return (size_t)(totalNumberOfClusters*sectorsPerCluster);
}

size_t SDFS::sectorSize()
{
    if(_pdrv == 0xFF) {
        return 0;
    }
    DWORD sectorsPerCluster;
    DWORD bytesPerSector;
    DWORD numberOfFreeClusters;
    DWORD totalNumberOfClusters;
    BOOL ok= GetDiskFreeSpaceA(NULL,&sectorsPerCluster,&bytesPerSector,&numberOfFreeClusters,&totalNumberOfClusters);
    if(!ok) {
        return 0;
    }
    return bytesPerSector;
}

uint64_t SDFS::totalBytes()
{
	return cardSize();
}

uint64_t SDFS::usedBytes()
{
	ULARGE_INTEGER FreeBytesAvailable = { 0 };
    ULARGE_INTEGER TotalNumberOfBytes={ 0 };
    ULARGE_INTEGER TotalNumberOfFreeBytes={ 0 };

    BOOL ok = GetDiskFreeSpaceEx(
        NULL,
        &FreeBytesAvailable,
        &TotalNumberOfBytes,
        &TotalNumberOfFreeBytes
        );

    if(!ok) {
        return 0;
    }
    return TotalNumberOfBytes.QuadPart-TotalNumberOfBytes.QuadPart;
}

bool SDFS::readRAW(uint8_t* buffer, uint32_t sector)
{
    return false;
}

bool SDFS::writeRAW(uint8_t* buffer, uint32_t sector)
{
    return false;
}


SDFS SD = SDFS(FSImplPtr(new StdioImpl));