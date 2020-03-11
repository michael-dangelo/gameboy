#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include <stdint.h>

void Cartridge_load(const char *filename);
uint8_t Cartridge_rb(uint16_t addr);
void Cartridge_wb(uint16_t addr, uint8_t val);
uint8_t *Cartridge_rawAddress(uint32_t addr);

#endif
