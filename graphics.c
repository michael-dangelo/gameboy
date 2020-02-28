#include "graphics.h"

#include "SDL/SDL.h"

#include <stdio.h>
#include <stdlib.h>

static SDL_Surface *screen = NULL;
static uint8_t lcdDisplayEnable = 0;
static uint8_t windowTileMapSelect = 0;
static uint8_t windowDisplayEnable = 0;
static uint8_t tileDataSelect = 0;
static uint8_t bgTileMapSelect = 0;
static uint8_t spriteSize = 0;
static uint8_t spriteDisplayEnable = 0;
static uint8_t bgDisplay = 0;

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

typedef enum {
    HBLANK,
    VBLANK,
    OAM,
    VRAM
} Mode;

static Mode mode = 0;
static uint16_t clock = 0;
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
            if (line < 143)
                mode = OAM;
            else
                mode = VBLANK;
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
}

uint8_t Graphics_rb()
{
    uint8_t res = (lcdDisplayEnable << 7) |
                  (windowTileMapSelect << 6) |
                  (windowDisplayEnable << 5) |
                  (tileDataSelect << 4) |
                  (bgTileMapSelect << 3) |
                  (spriteSize << 2) |
                  (spriteDisplayEnable << 1) |
                  (bgDisplay);
    printf("reading from gpu - %02x\n", res);
    return res;
}

void Graphics_wb(uint8_t val)
{
    lcdDisplayEnable = (val >> 7) & 1;
    windowTileMapSelect = (val >> 6) & 1;
    windowDisplayEnable = (val >> 5) & 1;
    tileDataSelect = (val >> 4) & 1;
    bgTileMapSelect = (val >> 3) & 1;
    spriteSize = (val >> 2) & 1;
    spriteDisplayEnable = (val >> 1) & 1;
    bgDisplay = val & 1;
    printf("writing to gpu - %02x", val);
}
