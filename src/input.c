#include "input.h"

#include "debug.h"

#include "SDL/SDL.h"

// Select buttons
uint8_t buttonsSelect = 1;

// Select direction keys
uint8_t directionsSelect = 1;

// Controls
uint8_t down = 1;
uint8_t start = 1;
uint8_t up = 1;
uint8_t select = 1;
uint8_t left = 1;
uint8_t b = 1;
uint8_t right = 1;
uint8_t a = 1;

void Input_pressed(SDL_Event *event)
{
    uint8_t pressed = event->type == SDL_KEYDOWN;

    switch (event->key.keysym.sym)
    {
        case SDLK_RETURN:
            start = !pressed;
            INPUT_PRINT(("start pressed %d\n", pressed));
            break;
        case SDLK_DOWN:
            down = !pressed;
            INPUT_PRINT(("down pressed %d\n", pressed));
            break;
        case SDLK_ESCAPE:
            select = !pressed;
            INPUT_PRINT(("select pressed %d\n", pressed));
            break;
        case SDLK_UP:
            up = !pressed;
            INPUT_PRINT(("up pressed %d\n", pressed));
            break;
        case SDLK_BACKSPACE:
            b = !pressed;
            INPUT_PRINT(("b pressed %d\n", pressed));
            break;
        case SDLK_LEFT:
            left = !pressed;
            INPUT_PRINT(("left pressed %d\n", pressed));
            break;
        case SDLK_SPACE:
            a = !pressed;
            INPUT_PRINT(("a pressed %d\n", pressed));
            break;
        case SDLK_RIGHT:
            right = !pressed;
            INPUT_PRINT(("right pressed %d\n", pressed));
            break;
    }
}

uint8_t Input_read(void)
{
    uint8_t downOrStart = !directionsSelect ? down : start;
    uint8_t upOrSelect = !directionsSelect ? up : select;
    uint8_t leftOrB = !directionsSelect ? left : b;
    uint8_t rightOrA = !directionsSelect ? right : a;
    uint8_t val = (downOrStart) << 3 |
                  (upOrSelect) << 2 |
                  (leftOrB) << 1 |
                  (rightOrA);
    INPUT_PRINT(("input read buttonsselect %d directionSelect %d val %02x\n",
        buttonsSelect, directionsSelect, val));
    return val;
}

void Input_write(uint8_t val)
{
    buttonsSelect = (val >> 5) & 1;
    directionsSelect = (val >> 4) & 1;
    INPUT_PRINT(("input write buttonsselect %d directionSelect %d val %02x\n",
        buttonsSelect, directionsSelect, val));
}
