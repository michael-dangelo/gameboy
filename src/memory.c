#include "memory.h"

#include "debug.h"
#include "graphics.h"
#include "timer.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
    #define strdup _strdup
    #define openFile(filePtr, fileName, flags) fopen_s(&filePtr, fileName, flags)
#else
    #define openFile(filePtr, fileName, flags) filePtr = fopen(fileName, flags)
#endif

#define RAM_SIZE 1 << 16
#define MAX_CART_SIZE 1 << 21

static uint8_t ram[RAM_SIZE];
static uint8_t cart[MAX_CART_SIZE];
static uint8_t inBootRom = 1;
static uint8_t bootRom[0x100] = {
    0x31, 0xFE, 0xFF, 0xAF, 0x21, 0xFF, 0x9F, 0x32, 0xCB, 0x7C, 0x20, 0xFB, 0x21, 0x26, 0xFF, 0x0E,
    0x11, 0x3E, 0x80, 0x32, 0xE2, 0x0C, 0x3E, 0xF3, 0xE2, 0x32, 0x3E, 0x77, 0x77, 0x3E, 0xFC, 0xE0,
    0x47, 0x11, 0x04, 0x01, 0x21, 0x10, 0x80, 0x1A, 0xCD, 0x95, 0x00, 0xCD, 0x96, 0x00, 0x13, 0x7B,
    0xFE, 0x34, 0x20, 0xF3, 0x11, 0xD8, 0x00, 0x06, 0x08, 0x1A, 0x13, 0x22, 0x23, 0x05, 0x20, 0xF9,
    0x3E, 0x19, 0xEA, 0x10, 0x99, 0x21, 0x2F, 0x99, 0x0E, 0x0C, 0x3D, 0x28, 0x08, 0x32, 0x0D, 0x20,
    0xF9, 0x2E, 0x0F, 0x18, 0xF3, 0x67, 0x3E, 0x64, 0x57, 0xE0, 0x42, 0x3E, 0x91, 0xE0, 0x40, 0x04,
    0x1E, 0x02, 0x0E, 0x0C, 0xF0, 0x44, 0xFE, 0x90, 0x20, 0xFA, 0x0D, 0x20, 0xF7, 0x1D, 0x20, 0xF2,
    0x0E, 0x13, 0x24, 0x7C, 0x1E, 0x83, 0xFE, 0x62, 0x28, 0x06, 0x1E, 0xC1, 0xFE, 0x64, 0x20, 0x06,
    0x7B, 0xE2, 0x0C, 0x3E, 0x87, 0xE2, 0xF0, 0x42, 0x90, 0xE0, 0x42, 0x15, 0x20, 0xD2, 0x05, 0x20,
    0x4F, 0x16, 0x20, 0x18, 0xCB, 0x4F, 0x06, 0x04, 0xC5, 0xCB, 0x11, 0x17, 0xC1, 0xCB, 0x11, 0x17,
    0x05, 0x20, 0xF5, 0x22, 0x23, 0x22, 0x23, 0xC9, 0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B,
    0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D, 0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E,
    0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99, 0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC,
    0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E, 0x3C, 0x42, 0xB9, 0xA5, 0xB9, 0xA5, 0x42, 0x3C,
    0x21, 0x04, 0x01, 0x11, 0xA8, 0x00, 0x1A, 0x13, 0xBE, 0x20, 0xFE, 0x23, 0x7D, 0xFE, 0x34, 0x20,
    0xF5, 0x06, 0x19, 0x78, 0x86, 0x23, 0x05, 0x20, 0xFB, 0x86, 0x20, 0xFE, 0x3E, 0x01, 0xE0, 0x50,
};

// FF0F - Interrupt Flag
uint8_t interruptFlag = 0;

// FFFF - Interrupt Enable
uint8_t interruptEnable = 0;

// MBC
uint8_t externalRamEnable = 0;
uint8_t romBankSelect = 1;
uint8_t ramBankSelect = 0;
uint8_t romRamModeSelect = 0;

void Mem_loadCartridge(const char *cartFilename)
{
    FILE *cartridge = NULL;
    openFile(cartridge, cartFilename, "rb");
    if (!cartridge)
    {
        printf("Failed to open rom %s\n", cartFilename);
        exit(1);
    }
    if (!fread(cart, 1, MAX_CART_SIZE, cartridge))
    {
        printf("Failed to read from cart\n");
        exit(1);
    }
    if (fclose(cartridge))
    {
        printf("Failed to close cart\n");
        exit(1);
    }
    uint8_t cartType = cart[0x147];
    assert(cartType == 0 || cartType == 1);
#ifdef SKIP_BOOTROM
    inBootRom = 0;
#endif
}

uint8_t Mem_rb(uint16_t addr)
{
    uint8_t *mem = ram;
    char *location = strdup("unknown");
    const uint8_t msb = (addr & 0xF000) >> 12;
    if (addr < 0x100 && inBootRom)
    {
        mem = bootRom;
        location = strdup("bootrom");
    }
    else if (msb < 0x4)
    {
        mem = cart;
        location = strdup("cart rom bank 0");
    }
    else if (msb < 0x8)
    {
        MEM_PRINT(("mem read cartrom addr %04x rom bank %d\n", addr, romBankSelect));
        uint32_t cartAddr = addr + (0x4000 * (romBankSelect - 1));
        return cart[cartAddr];
    }
    else if (msb < 0xA)
    {
        // vram
        free(location);
        return Graphics_rb(addr);
    }
    else if (msb < 0xC)
    {
        location = strdup("externalram");
    }
    else if (msb < 0xE)
    {
        location = strdup("workingram");
    }
    else if (msb < 0xF || addr < 0xFE00)
    {
        addr -= 0x2000;
        location = strdup("workingramecho");
    }
    else if (addr < 0xFEA0)
    {
        location = strdup("oam");
    }
    else if (addr < 0xFF00)
    {
        MEM_PRINT(("read from unusable memory\n"));
        return 0xFF;
    }
    else if (0xFF04 <= addr && addr <= 0xFF07)
    {
        free(location);
        return Timer_rb(addr);
    }
    else if (addr == 0xFF0F)
    {
        free(location);
        uint8_t timerInterrupt = Timer_interrupt();
        interruptFlag |= timerInterrupt << 2;
        MEM_PRINT(("mem read interrupt flag, val %02x\n", interruptFlag));
        return interruptFlag;
    }
    else if (addr == 0xFF24)
    {
        location = strdup("volumecontrol");
    }
    else if (addr == 0xFF25)
    {
        location = strdup("soundoutputselect");
    }
    else if (addr == 0xFF26)
    {
        location = strdup("soundenable");
    }
    else if ((0xFF40 <= addr && addr <= 0xFF44) || addr == 0xFF47)
    {
        return Graphics_rb(addr);
    }
    else if (addr < 0xFF80)
    {
        location = strdup("ioports");
    }
    else if (addr < 0xFFFF)
    {
        location = strdup("highram");
    }
    else if (addr == 0xFFFF)
    {
        MEM_PRINT(("mem read interrupt enable, val %02x\n", interruptEnable));
        free(location);
        return interruptEnable;
    }
    uint8_t val = mem[addr];
    MEM_PRINT(("%s read byte at %04x, val %02x\n", location, addr, val));
    free(location);
    return val;
}

uint16_t Mem_rw(uint16_t addr)
{
    uint16_t val = Mem_rb(addr) + ((uint16_t)Mem_rb(addr + 1) << 8);
    MEM_PRINT(("mem read word at %04x, val %04x\n", addr, val));
    return val;
}

void Mem_wb(uint16_t addr, uint8_t val)
{
    char *location = strdup("unknown");
    const uint8_t msb = (addr & 0xF000) >> 12;
    if (msb < 0x2)
    {
        MEM_PRINT(("mem write to external ram enable, val %02x\n", val));
        externalRamEnable = val != 0;
        return;
    }
    else if (msb < 0x4)
    {
        MEM_PRINT(("mem write to rom bank select, val %02x\n", val));
        romBankSelect = val & 0x1F;
        if (romBankSelect == 0)
            romBankSelect++;
        return;
    }
    else if (msb < 0x6)
    {
        MEM_PRINT(("mem write to ram bank select or upper bits of rom bank, val %02x\n", val));
        ramBankSelect = val & 3;
        return;
    }
    else if (msb < 0x8)
    {
        MEM_PRINT(("mem write to rom/ram mode select, val %02x\n", val));
        romRamModeSelect = val & 1;
        return;
    }
    else if (msb < 0xA)
    {
        // vram
        Graphics_wb(addr, val);
        free(location);
        return;
    }
    else if (msb < 0xC)
    {
        location = strdup("externalram");
    }
    else if (msb < 0xE)
    {
        location = strdup("workingram");
    }
    else if (msb < 0xF || addr < 0xFE00)
    {
        addr -= 0x2000;
        location = strdup("workingramecho");
    }
    else if (addr < 0xFEA0)
    {
        location = strdup("oam");
    }
    else if (addr < 0xFF00)
    {
        MEM_PRINT(("write to unusable memory\n"));
        return;
    }
    else if (0xFF04 <= addr && addr <= 0xFF07)
    {
        Timer_wb(addr, val);
        free(location);
        return;
    }
    else if (addr == 0xFF0F)
    {
        MEM_PRINT(("mem write interrupt flag, val %02x\n", val));
        free(location);
        interruptFlag = val;
        return;
    }
    else if (addr == 0xFF24)
    {
        location = strdup("volumecontrol");
    }
    else if (addr == 0xFF25)
    {
        location = strdup("soundoutputselect");
    }
    else if (addr == 0xFF26)
    {
        location = strdup("soundenable");
    }
    else if ((0xFF40 <= addr && addr <= 0xFF44) || addr == 0xFF47)
    {
        Graphics_wb(addr, val);
        free(location);
        return;
    }
    else if (addr == 0xFF50)
    {
        location = strdup("disablebootromregister");
        inBootRom = !val;
    }
    else if (addr < 0xFF80)
    {
        location = strdup("ioports");
    }
    else if (addr < 0xFFFF)
    {
        location = strdup("highram");
    }
    else if (addr == 0xFFFF)
    {
        MEM_PRINT(("mem write interrupt enable, val %02x\n", val));
        free(location);
        interruptEnable = val;
        return;

    }
    MEM_PRINT(("%s write byte at %04x val %02x\n", location, addr, val));
    free(location);
    ram[addr] = val;
}

void Mem_ww(uint16_t addr, uint16_t val)
{
    Mem_wb(addr, val & 255);
    Mem_wb(addr + 1, val >> 8);
    MEM_PRINT(("mem write word at %04x val %04x\n", addr, val));
}
