#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

void Timer_step(uint8_t ticks);
uint8_t Timer_rb(uint16_t addr);
void Timer_wb(uint16_t addr, uint8_t val);
uint8_t Timer_interrupt(void);

#endif
