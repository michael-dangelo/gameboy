#ifndef INPUT_H
#define INPUT_H

#include "SDL/SDL.h"

#include <stdint.h>

void Input_pressed(SDL_Event *e);
uint8_t Input_read(void);
void Input_write(uint8_t val);
uint8_t Input_interrupt(void);

#endif
