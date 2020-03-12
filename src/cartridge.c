#include "cartridge.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

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

static const uint16_t romBanksCfgs[8] =
{
    0, 4, 8, 16, 32, 64, 128, 256
};

static const uint16_t externalRamSizes[4] =
{
    0, 2048, 8192, 32768
};

static char cartName[101];
static CartridgeType cartType = 0;
static uint16_t romBanks = 0;
static uint16_t ramSize = 0;

static uint8_t externalRamEnable = 0;
static uint8_t romBankSelect = 0;
static uint8_t ramBankSelect = 0;
static uint8_t romRamModeSelect = 0;

static struct tm timeRegister = {};

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

uint8_t isMBC3(uint8_t cartType)
{
    return cartType == MBC3 ||
           cartType == MBC3_RAM ||
           cartType == MBC3_RAM_BATTERY;
}

uint8_t isBatteryRam(uint8_t cartType)
{
    return cartType == MBC1_RAM_BATTERY ||
           cartType == MBC3_RAM_BATTERY;
}

uint32_t translateMBCRomAddr(uint16_t addr)
{
    uint8_t bankSelect = romBankSelect;
    if (!bankSelect)
        bankSelect++;
    if (isMBC1(cartType) && !romRamModeSelect)
        bankSelect |= (ramBankSelect << 5);
    if (!romBanks)
        bankSelect = 1;
    else if (bankSelect > romBanks)
        bankSelect = romBanks;
    return addr + (0x4000 * (bankSelect - 1));
}

uint32_t translateMBCRamAddr(uint16_t addr)
{
    uint8_t bankSelect = (isMBC1(cartType) && !romRamModeSelect) ? 0 : ramBankSelect;
    assert(bankSelect <= 3);
    return addr - 0xA000 + (0x2000 * bankSelect);
}

void latchTime(void)
{
    time_t rawTime;
    time(&rawTime);
    timeRegister = *localtime(&rawTime);
}

uint8_t clockValue()
{
    switch (ramBankSelect)
    {
        case 0x8:
            return timeRegister.tm_sec;
        case 0x9:
            return timeRegister.tm_min;
        case 0xA:
            return timeRegister.tm_hour;
        case 0xB:
            return timeRegister.tm_yday & 0xFF;
        case 0xC:
            return 0;
    }
    assert(0);
    return 0;
}

void loadSaveFile(void);

void Cartridge_load(const char *filename)
{
    strncpy(cartName, filename, 100);
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
    romBanks = romBanksCfgs[cart[0x148]];
    ramSize = externalRamSizes[cart[0x149]];
    assert(isSupportedCartridge(cartType));
    loadSaveFile();
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
        if (isMBC3(cartType) && ramBankSelect >= 0x8)
            return clockValue();
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
        externalRamEnable = (val & 0xF) == 0xA;
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
        ramBankSelect = val;
        return;
    }
    else if (addr < 0x8000)
    {
        if (isMBC3(cartType) && !romRamModeSelect && val)
            latchTime();
        romRamModeSelect = val & 1;
        return;
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

void saveFileName(char buffer[])
{
    for (uint8_t i = 0; i < 101; i++)
    {
        if (i == 98)
            return;
        char a = cartName[i];
        if (a == '\0')
            return;
        if (a == '.')
        {
            strncpy(buffer + i, ".sav", 5);
            break;
        }
        buffer[i] = cartName[i];
    }
}

void loadSaveFile(void)
{
    char saveName[101] = {};
    saveFileName(saveName);
    FILE *saveFile = fopen(saveName, "rb");
    if (!saveFile)
        return;
    if (!fread(externalRam, 1, ramSize, saveFile))
    {
        printf("Failed to read save file\n");
        exit(1);
    }
    if (fclose(saveFile))
    {
        printf("Failed to close save file\n");
        exit(1);
    }
}

void Cartridge_writeSaveFile(void)
{
    if (!(isBatteryRam(cartType) && ramSize > 0))
        return;
    char saveName[101] = {};
    saveFileName(saveName);
    FILE *saveFile = fopen(saveName, "wb");
    if (!saveFile)
    {
        printf("Failed to open save file %s\n", saveName);
        exit(1);
    }
    if (!fwrite(externalRam, 1, ramSize, saveFile))
    {
        printf("Failed to write to save file\n");
        exit(1);
    }
    if (fclose(saveFile))
    {
        printf("Failed to close save file\n");
        exit(1);
    }
}
