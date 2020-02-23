#include "graphics.h"

#include "SDL/SDL.h"

#include <stdio.h>
#include <stdlib.h>

static SDL_Surface *screen = NULL;

void Graphics_init(void)
{
    if (SDL_Init(SDL_INIT_VIDEO))
    {
        printf("Failed to initialize SDL\n");
        exit(1);
    }
    atexit(SDL_Quit);
    SDL_Window *window = SDL_CreateWindow("Gameboy", 300, 300, 160, 144, SDL_WINDOW_SHOWN);
    if (!window)
    {
        printf("Failed to create SDL window\n");
        exit(1);
    }
    screen = SDL_GetWindowSurface(window);
    if (!screen)
    {
        printf("Failed to get window surface\n");
        exit(1);
    }
}

enum Mode {
    HBLANK,
    VBLANK,
    OAM,
    VRAM
};

static uint16_t clock = 0;
static uint8_t mode = 0;
static uint8_t line = 0;

static void step(uint8_t ticks)
{
    clock += ticks;
    switch (mode)
    {
        case HBLANK:
            if (clock < 204)
                return;
            clock = 0;
            line++;
            if (line == 143)
            {
                mode = VBLANK;
                // TODO: blit image onto window
            }
            else
            {
                mode = OAM;
            }
            break;
        case VBLANK:
            if (clock < 4560)
                return;
            clock = 0;
            line = 0;
            mode = OAM;
            break;
        case OAM:
            if (clock < 80)
                return;
            clock = 0;
            mode = VRAM;
            break;
        case VRAM:
            if (clock < 172)
                return;
            clock = 0;
            mode = HBLANK;
            // TODO: render current scanline
            break;
    }
    return;
}

void Graphics_step(uint8_t ticks)
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
        if (event.type == SDL_QUIT)
            exit(0);
    step(ticks);
    printf("graphics - clock %d mode %d\n", clock, mode);
    SDL_Delay(1000);
}
