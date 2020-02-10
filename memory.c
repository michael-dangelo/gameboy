#include "memory.h"

#include <stdint.h>
#include <stdio.h>

static uint8_t ram[1 << 16];

void loadBootRom()
{
    FILE *ptr = fopen("boot.bin", "rb");
    fread(ram, 256, 1, ptr);
    fclose(ptr);
}

uint8_t getByte(uint16_t addr)
{
    return ram[addr];
}

void writeByte(uint16_t addr, uint8_t val)
{
    ram[addr] = val;
}

