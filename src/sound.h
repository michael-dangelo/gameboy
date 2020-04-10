#ifndef SOUND_H
#define SOUND_H

#include <stdint.h>

void Sound_init(void);
void Sound_step(uint8_t ticks);
uint8_t Sound_rb(uint16_t addr);
void Sound_wb(uint16_t addr, uint8_t val);

#endif
