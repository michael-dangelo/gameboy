#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stdint.h>

void Graphics_init(void);
void Graphics_step(uint8_t ticks);
uint8_t Graphics_rb();
void Graphics_wb(uint8_t val);

#endif
