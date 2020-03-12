#include "input.h"

#include "debug.h"

#include "SDL/SDL.h"

// Select buttons
static uint8_t buttonsSelect = 1;

// Select direction keys
static uint8_t directionsSelect = 1;

// Controls
static uint8_t down = 1;
static uint8_t up = 1;
static uint8_t left = 1;
static uint8_t right = 1;
static uint8_t start = 1;
static uint8_t select = 1;
static uint8_t b = 1;
static uint8_t a = 1;

enum Control
{
    DOWN,
    UP,
    LEFT,
    RIGHT,
    START,
    SELECT,
    A,
    B,
    NUM_CONTROLS
};

static SDL_Keycode controlMapping[NUM_CONTROLS] =
{
    SDLK_DOWN,
    SDLK_UP,
    SDLK_LEFT,
    SDLK_RIGHT,
    SDLK_RETURN,
    SDLK_ESCAPE,
    SDLK_1,
    SDLK_2
};

// Joypad Interrupt
static uint8_t interruptRequest = 0;

void Input_pressed(SDL_Event *event)
{
    uint8_t pressed = event->type == SDL_KEYDOWN;
    SDL_Keycode key = event->key.keysym.sym;
    if (controlMapping[DOWN] == key)
    {
        down = !pressed;
        if (pressed && directionsSelect)
            interruptRequest = 1;
        INPUT_PRINT(("down pressed %d\n", pressed));
    }
    else if (controlMapping[UP] == key)
    {
        up = !pressed;
        if (pressed && directionsSelect)
            interruptRequest = 1;
        INPUT_PRINT(("up pressed %d\n", pressed));
    }
    else if (controlMapping[LEFT] == key)
    {
        left = !pressed;
        if (pressed && directionsSelect)
            interruptRequest = 1;
        INPUT_PRINT(("left pressed %d\n", pressed));
    }
    else if (controlMapping[RIGHT] == key)
    {
        right = !pressed;
        if (pressed && directionsSelect)
            interruptRequest = 1;
        INPUT_PRINT(("right pressed %d\n", pressed));
    }
    else if (controlMapping[START] == key)
    {
        start = !pressed;
        if (pressed && buttonsSelect)
            interruptRequest = 1;
        INPUT_PRINT(("start pressed %d\n", pressed));
    }
    else if (controlMapping[SELECT] == key)
    {
        select = !pressed;
        if (pressed && buttonsSelect)
            interruptRequest = 1;
        INPUT_PRINT(("select pressed %d\n", pressed));
    }
    else if (controlMapping[A] == key)
    {
        a = !pressed;
        if (pressed && buttonsSelect)
            interruptRequest = 1;
        INPUT_PRINT(("a pressed %d\n", pressed));
    }
    else if (controlMapping[B] == key)
    {
        b = !pressed;
        if (pressed && buttonsSelect)
            interruptRequest = 1;
        INPUT_PRINT(("b pressed %d\n", pressed));
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

uint8_t Input_interrupt(void)
{
    uint8_t interrupt = interruptRequest;
    interruptRequest = 0;
    return interrupt;
}
