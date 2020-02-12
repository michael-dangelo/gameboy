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
    uint8_t val = ram[addr];
    printf("mem read byte at %04x, val %02x\n", addr, val);
    return val;
}

uint16_t Mem_rw(uint16_t addr)
{
    uint16_t val = ram[addr] + ((uint16_t)ram[addr + 1] << 8);
    printf("mem read word at %04x, val %02x\n", addr, val);
    return val;
}

void Mem_wb(uint16_t addr, uint8_t val)
{
    printf("mem write byte at %04x val %02x\n", addr, val);
    ram[addr] = val;
}

void Mem_ww(uint16_t addr, uint16_t val)
{
    printf("mem write word at %04x val %02x\n", addr, val);
    ram[addr] = val & 255;
    ram[addr + 1] = val >> 8;
}

