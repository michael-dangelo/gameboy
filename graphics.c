#include "graphics.h"

#include "SDL/SDL.h"

#include <stdio.h>
#include <stdlib.h>

static SDL_Surface *screen = NULL;

// FF40 - LCD control
static uint8_t lcdDisplayEnable = 0;
static uint8_t windowTileMapSelect = 0;
static uint8_t windowDisplayEnable = 0;
static uint8_t tileDataSelect = 0;
static uint8_t bgTileMapSelect = 0;
static uint8_t spriteSize = 0;
static uint8_t spriteDisplayEnable = 0;
static uint8_t bgDisplay = 0;

// FF42 - Scroll Y
static uint8_t scrollY;

// FF43 - Scrool X
static uint8_t scrollX;

// FF44 - LDC Y-Coordinate
static uint8_t line = 0;

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
    SDL_memset(screen->pixels, 0, screen->h * screen->pitch);
    SDL_UpdateWindowSurface(window);
}

typedef enum {
    HBLANK,
    VBLANK,
    OAM,
    VRAM
} Mode;

static Mode mode = 0;
static uint16_t clock = 0;

static void renderScanline()
{

}

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
            if (clock < 456)
                return;
            clock = 0;
            line++;
            if (line <= 153)
                return;
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
            renderScanline();
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
    // printf("graphics - clock %d mode %d line %d\n", clock, mode, line);
}

uint8_t Graphics_rb(uint16_t addr)
{
    uint8_t res = 0;
    switch (addr)
    {
        case 0xFF40:
            res = (lcdDisplayEnable << 7) |
                  (windowTileMapSelect << 6) |
                  (windowDisplayEnable << 5) |
                  (tileDataSelect << 4) |
                  (bgTileMapSelect << 3) |
                  (spriteSize << 2) |
                  (spriteDisplayEnable << 1) |
                  (bgDisplay);
            break;
        case 0xFF42:
            res = scrollY;
            break;
        case 0xFF43:
            res = scrollX;
            break;
        case 0xFF44:
            res = line;
            break;
    }
    // printf("reading from gpu addr %02x val %02x\n", addr, res);
    return res;
}

void Graphics_wb(uint16_t addr, uint8_t val)
{
    // printf("writing to gpu addr %02x val %02x", addr, val);
    switch (addr)
    {
        case 0xFF40:
            lcdDisplayEnable = (val >> 7) & 1;
            windowTileMapSelect = (val >> 6) & 1;
            windowDisplayEnable = (val >> 5) & 1;
            tileDataSelect = (val >> 4) & 1;
            bgTileMapSelect = (val >> 3) & 1;
            spriteSize = (val >> 2) & 1;
            spriteDisplayEnable = (val >> 1) & 1;
            bgDisplay = val & 1;
            break;
        case 0xFF42:
            scrollY = val;
        case 0xFF43:
            scrollX = val;
        case 0xFF44:
            // Writing to the LCD Y-Coordinate register (line) resets the counter.
            line = 0;
            break;
    }
}
