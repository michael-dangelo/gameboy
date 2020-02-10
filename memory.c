#include "memory.h"

#include <stdint.h>
#include <stdio.h>

static uint8_t ram[1 << 16];

void Mem_loadBootRom()
{
    FILE *ptr = fopen("boot.bin", "rb");
    fread(ram, 256, 1, ptr);
    fclose(ptr);
}

uint8_t Mem_rb(uint16_t addr)
{
    return ram[addr];
}

uint16_t Mem_rw(uint16_t addr)
{
    return ((uint16_t)ram[addr] << 8) + ram[addr + 1];
}

void Mem_wb(uint16_t addr, uint8_t val)
{
    ram[addr] = val;
}

void Mem_ww(uint16_t addr, uint16_t val)
{
    ram[addr] = val >> 8;
    ram[addr + 1] = val & 255;
}

