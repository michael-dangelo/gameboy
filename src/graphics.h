#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stdint.h>

void Graphics_init(void);
void Graphics_step(uint8_t ticks);
uint8_t Graphics_rb(uint16_t addr);
void Graphics_wb(uint16_t addr, uint8_t val);
uint8_t Graphics_vblankInterrupt(void);

#endif
