#include "memory.h"

#include "cartridge.h"
#include "debug.h"
#include "graphics.h"
#include "input.h"
#include "timer.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#define RAM_SIZE 1 << 16
static uint8_t ram[RAM_SIZE];

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

void Memory_init(void)
{
#ifdef SKIP_BOOTROM
    inBootRom = 0;
#endif
}

uint8_t Mem_rb(uint16_t addr)
{
    uint8_t val = 0;
    if (addr < 0x100 && inBootRom)
    {
        val = bootRom[addr];
        MEM_READ("bootrom", addr, val);
    }
    else if (addr < 0x8000)
    {
        val = Cartridge_rb(addr);
        MEM_READ("cart rom", addr, val);
    }
    else if (addr < 0xA000)
    {
        val = Graphics_rb(addr);
        MEM_READ("gpu vram", addr, val);
    }
    else if (addr < 0xC000)
    {
        val = Cartridge_rb(addr);
        MEM_READ("external ram", addr, val);
    }
    else if (addr < 0xE000)
    {
        val = ram[addr];
        MEM_READ("working ram", addr, val);
    }
    else if (addr < 0xFE00)
    {
        val = ram[addr - 0x2000];
        MEM_READ("working ram echo", addr, val);
    }
    else if (addr < 0xFEA0)
    {
        val = Graphics_rb(addr);
        MEM_READ("gpu oam", addr, val);
    }
    else if (addr < 0xFF00)
    {
        val = 0xFF;
        MEM_READ("unusable memory", addr, val);
    }
    else if (addr == 0xFF00)
    {
        val = Input_read();
        MEM_READ("joypad", addr, val);
    }
    else if (0xFF01 <= addr && addr <= 0xFF02)
    {
        val = ram[addr];
        MEM_READ("serial ports", addr, val);
    }
    else if (0xFF04 <= addr && addr <= 0xFF07)
    {
        val = Timer_rb(addr);
        MEM_READ("joypad", addr, val);
    }
    else if (addr == 0xFF0F)
    {
        uint8_t vblankInterrupt = Graphics_vblankInterrupt();
        uint8_t lcdStatusInterrupt = Graphics_statusInterrupt();
        uint8_t timerInterrupt = Timer_interrupt();
        uint8_t joypadInterrupt = Input_interrupt();
        interruptFlag |= vblankInterrupt;
        interruptFlag |= lcdStatusInterrupt << 1;
        interruptFlag |= timerInterrupt << 2;
        interruptFlag |= joypadInterrupt << 4;
        val = interruptFlag;
        MEM_READ("interrupt flag", addr, val);
    }
    else if (addr == 0xFF24)
    {
        val = ram[addr];
        MEM_READ("volume control", addr, val);
    }
    else if (addr == 0xFF25)
    {
        val = ram[addr];
        MEM_READ("sound output select", addr, val);
    }
    else if (addr == 0xFF26)
    {
        val = ram[addr];
        MEM_READ("sound enable", addr, val);
    }
    else if ((0xFF40 <= addr && addr <= 0xFF45) || (0xFF47 <= addr && addr <= 0xFF49))
    {
        val = Graphics_rb(addr);
        MEM_READ("gpu registers", addr, val);
    }
    else if (addr == 0xFF46)
    {
        printf("attempted read from dma request\n");
        assert(0);
    }
    else if (addr < 0xFF80)
    {
        val = ram[addr];
        MEM_READ("unknown io port", addr, val);
    }
    else if (addr < 0xFFFF)
    {
        val = ram[addr];
        MEM_READ("high ram", addr, val);
    }
    else if (addr == 0xFFFF)
    {
        MEM_READ("interrupt enable", addr, val);
        val = interruptEnable;
    }
    return val;
}

uint16_t Mem_rw(uint16_t addr)
{
    return Mem_rb(addr) + ((uint16_t)Mem_rb(addr + 1) << 8);
}

void Mem_wb(uint16_t addr, uint8_t val)
{
    if (addr < 0x2000)
    {
        Cartridge_wb(addr, val);
        MEM_WRITE("external ram enable", addr, val);
    }
    else if (addr < 0x4000)
    {
        Cartridge_wb(addr, val);
        MEM_WRITE("rom bank select", addr, val);
    }
    else if (addr < 0x6000)
    {
        Cartridge_wb(addr, val);
        MEM_WRITE("ram bank select/upper bits of rom bank select", addr, val);
    }
    else if (addr < 0x8000)
    {
        Cartridge_wb(addr, val);
        MEM_WRITE("rom/ram mode select", addr, val);
    }
    else if (addr < 0xA000)
    {
        Graphics_wb(addr, val);
        MEM_WRITE("gpu vram", addr, val);
    }
    else if (addr < 0xC000)
    {
        Cartridge_wb(addr, val);
        MEM_WRITE("external ram", addr, val);
    }
    else if (addr < 0xE000)
    {
        ram[addr] = val;
        MEM_WRITE("working ram", addr, val);
    }
    else if (addr < 0xFE00)
    {
        ram[addr - 0x2000] = val;
        MEM_WRITE("working ram echo", addr, val);
    }
    else if (addr < 0xFEA0)
    {
        Graphics_wb(addr, val);
        MEM_WRITE("gpu oam", addr, val);
    }
    else if (addr < 0xFF00)
    {
        MEM_WRITE("unusable memory", addr, val);
    }
    else if (addr == 0xFF00)
    {
        Input_write(val);
        MEM_WRITE("joypad", addr, val);
    }
    else if (0xFF01 <= addr && addr <= 0xFF02)
    {
        ram[addr] = val;
        MEM_WRITE("serial ports", addr, val);
    }
    else if (0xFF04 <= addr && addr <= 0xFF07)
    {
        Timer_wb(addr, val);
        MEM_WRITE("timer", addr, val);
    }
    else if (addr == 0xFF0F)
    {
        interruptFlag = val;
        MEM_WRITE("interrupt flag", addr, val);
        INT_PRINT(("interrupt flag written, val %02x\n", val));
    }
    else if (addr == 0xFF24)
    {
        ram[addr] = val;
        MEM_WRITE("volume control", addr, val);
    }
    else if (addr == 0xFF25)
    {
        ram[addr] = val;
        MEM_WRITE("sound output select", addr, val);
    }
    else if (addr == 0xFF26)
    {
        ram[addr] = val;
        MEM_WRITE("sound enable", addr, val);
    }
    else if ((0xFF40 <= addr && addr <= 0xFF45) || (0xFF47 <= addr && addr <= 0xFF49))
    {
        Graphics_wb(addr, val);
        MEM_WRITE("gpu registers", addr, val);
    }
    else if (addr == 0xFF46)
    {
        uint16_t addr = (uint16_t)val << 8;
        const uint8_t *dmaAddress = 0;
        const uint8_t msb = (addr & 0xF000) >> 12;
        if (msb < 0x8)
        {
            dmaAddress = Cartridge_rawAddress(addr);
        }
        else if (msb < 0xA)
        {
            printf("attempted dma from vram\n");
            assert(0);
        }
        else if (addr <= 0xF100)
        {
            if (msb >= 0xE)
                addr -= 0x2000;
            dmaAddress = ram + addr;
        }
        Graphics_dma(dmaAddress);
        MEM_WRITE("dma request", addr, val);
    }
    else if (addr == 0xFF50)
    {
        inBootRom = !val;
        MEM_WRITE("disable bootrom register", addr, val);
    }
    else if (addr < 0xFF80)
    {
        ram[addr] = val;
        MEM_WRITE("unknown io port", addr, val);
    }
    else if (addr < 0xFFFF)
    {
        ram[addr] = val;
        MEM_WRITE("high ram", addr, val);
    }
    else if (addr == 0xFFFF)
    {
        interruptEnable = val;
        MEM_WRITE("interrupt enable", addr, val);
    }
}

void Mem_ww(uint16_t addr, uint16_t val)
{
    Mem_wb(addr, val & 255);
    Mem_wb(addr + 1, val >> 8);
}
