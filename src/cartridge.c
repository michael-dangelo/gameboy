#include "cartridge.h"

#include <assert.h>
#include <stdio.h>

#define MAX_CART_SIZE 1 << 21
#define MAX_EXTERNAL_RAM_SIZE 1 << 15

static uint8_t cart[MAX_CART_SIZE];
static uint8_t externalRam[MAX_EXTERNAL_RAM_SIZE];

// MBC
typedef enum
{
    ROM_ONLY = 0x00,
    MBC1 = 0x01,
    MBC1_RAM = 0x02,
    MBC1_RAM_BATTERY = 0x03,
    MBC3 = 0x11,
    MBC3_RAM = 0x12,
    MBC3_RAM_BATTERY = 0x13
} CartridgeType;

#define NUM_SUPPORTED 7
static const CartridgeType supported[NUM_SUPPORTED] =
{
    ROM_ONLY,
    MBC1, MBC1_RAM, MBC1_RAM_BATTERY,
    MBC3, MBC3_RAM, MBC3_RAM_BATTERY
};

CartridgeType cartType = 0;

uint8_t externalRamEnable = 0;
uint8_t romBankSelect = 0;
uint8_t ramBankSelect = 0;
uint8_t romRamModeSelect = 0;
uint8_t clockLatch = 0;

uint8_t isSupportedCartridge(uint8_t cartType)
{
    for (uint8_t i = 0; i < NUM_SUPPORTED; i++)
        if (cartType == supported[i])
            return 1;
    printf("unsupported cartridge type %02x\n", cartType);
    return 0;
}

uint8_t isMBC1(uint8_t cartType)
{
    return cartType == ROM_ONLY ||
           cartType == MBC1 ||
           cartType == MBC1_RAM ||
           cartType == MBC1_RAM_BATTERY;
}

uint32_t translateMBCRomAddr(uint16_t addr)
{
    uint8_t bankSelect = romBankSelect;
    if (!bankSelect)
        bankSelect++;
    if (isMBC1(cartType) && !romRamModeSelect)
        bankSelect |= (ramBankSelect << 5);
    return addr + (0x4000 * (bankSelect - 1));
}

uint32_t translateMBCRamAddr(uint16_t addr)
{
    uint8_t bankSelect = (isMBC1(cartType) && !romRamModeSelect) ? 0 : ramBankSelect;
    assert(bankSelect <= 3);
    return addr - 0xA000 + (0x2000 * bankSelect);
}
void Cartridge_load(const char *filename)
{
    FILE *cartridge = fopen(filename, "rb");
    if (!cartridge)
    {
        printf("Failed to open rom %s\n", filename);
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
    cartType = cart[0x147];
    assert(isSupportedCartridge(cartType));
}

uint8_t Cartridge_rb(uint16_t addr)
{
    if (addr < 0x4000)
    {
        return cart[addr];
    }
    else if (addr < 0x8000)
    {
        return cart[translateMBCRomAddr(addr)];
    }
    else if (addr < 0xC000)
    {
        if (!externalRamEnable)
            return 0xFF;
        return externalRam[translateMBCRamAddr(addr)];
    }
    assert(0);
    return 0;
}

void Cartridge_wb(uint16_t addr, uint8_t val)
{
    if (addr < 0x2000)
    {
        externalRamEnable = val != 0;
        return;
    }
    else if (addr < 0x4000)
    {
        romBankSelect = val & 0x7F;
        if (isMBC1(cartType))
            romBankSelect &= 0x1F;
        return;
    }
    else if (addr < 0x6000)
    {
        ramBankSelect = val & 3;
        return;
    }
    else if (addr < 0x8000)
    {
        if (isMBC1(cartType))
        {
            romRamModeSelect = val & 1;
            return;
        }
        // TODO: RTC implementation here
    }
    else if (addr < 0xC000)
    {
        if (!externalRamEnable)
            return;
        externalRam[translateMBCRamAddr(addr)] = val;
        return;
    }
    assert(0);
    return;
}

uint8_t *Cartridge_rawAddress(uint32_t addr)
{
    if (addr < 0x4000)
        return cart + addr;
    return cart + translateMBCRomAddr(addr);
}
