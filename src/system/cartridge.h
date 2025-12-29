#ifndef __CARTRIDGE_H__
#define __CARTRIDGE_H__

#define SRAM_SIZE 0x8000

void cartridgeInit();

void romCopy(const void* romAddr, void* ramAddr, const int size);
void romCopyAsync(const void* romAddr, void* ramAddr, const int size);
void romCopyAsyncDrain();

void sramWrite(void* sramAddr, const void* ramAddr, const int size);
int sramRead(const void* sramAddr, void* ramAddr, const int size);

#define CALC_SEGMENT_POINTER(segmentedAddress, baseAddress) (void*)(((unsigned)(segmentedAddress) & 0xFFFFFF) + (baseAddress))

#endif
