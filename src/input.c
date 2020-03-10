#include "input.h"

#include "debug.h"

// Select buttons
uint8_t buttonsSelect = 1;

// Select direction keys
uint8_t directionsSelect = 1;

// Down or Start
uint8_t downOrStart = 1;

// Up or Select
uint8_t upOrSelect = 1;

// Left or B
uint8_t leftorB = 1;

// Right or A
uint8_t rightOrA = 1;

uint8_t Input_read(void)
{
    uint8_t val = ((downOrStart >> 3) & 1) |
                  ((upOrSelect >> 2) & 1) |
                  ((leftorB >> 1) & 1) |
                  rightOrA;
    INPUT_PRINT(("input read, val %02x\n", val));
    return val;
}

void Input_write(uint8_t val)
{
    buttonsSelect = (val >> 5) & 1;
    directionsSelect = (val >> 4) & 1;
    INPUT_PRINT(("input write, val %02x\n", val));
}
