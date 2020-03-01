#include "graphics.h"

#include "SDL/SDL.h"

#include <stdio.h>
#include <stdlib.h>

static SDL_Window *window = NULL;
static SDL_Surface *screen = NULL;
static SDL_Renderer *renderer = NULL;

static uint8_t vram[0x2000];

typedef enum {
    HBLANK,
    VBLANK,
    OAM,
    VRAM
} Mode;

static Mode mode = OAM;
static uint16_t clock = 0;

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
static uint8_t scrollY = 0;

// FF43 - Scrool X
static uint8_t scrollX = 0;

// FF44 - LDC Y-Coordinate
static uint8_t line = 0;

// FF47 - BG Palette Data
static uint8_t palette = 0;

void Graphics_init(void)
{
    if (SDL_Init(SDL_INIT_VIDEO))
    {
        printf("Failed to initialize SDL\n");
        exit(1);
    }
    atexit(SDL_Quit);
    window = SDL_CreateWindow("Gameboy", 300, 300, 160, 144, SDL_WINDOW_SHOWN);
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
    renderer = SDL_CreateRenderer(window, -1, 0);
    if (!renderer)
    {
        printf("Failed to create renderer\n");
        exit(1);
    }
}

static uint16_t tileLineAt(uint16_t addr, uint16_t yOffset)
{
    uint16_t tile = tileDataSelect ? (uint16_t)vram[addr] * 16
                                   : 0x1000 + ((int16_t)vram[addr] * 16);
    uint16_t pixels = tile + yOffset;
    return vram[pixels] + ((uint16_t)vram[pixels + 1] << 8);
}

static uint8_t colorAt(uint16_t tileLine, uint8_t x)
{
    uint8_t l = (tileLine >> 8) & 0xFF;
    uint8_t h = tileLine & 0xFF;
    uint8_t pixelOffset = 7 - x; // leftmost pixel is 7th bit
    uint8_t colorIndex = ((l >> pixelOffset) & 1) | (((h >> pixelOffset) & 1) << 1);
    if (colorIndex > 3)
    {
        printf("bad color value");
        exit(1);
    }
    uint8_t color = (palette >> colorIndex) & 3;
    static uint8_t colors[4] = {235, 192, 96, 0};
    return colors[color];
}

static void renderScanline()
{
    uint16_t tileMap = bgTileMapSelect ? 0x1C00 : 0x1800;
    uint16_t tileMapOffset = tileMap + (((line + scrollY) / 8) * 32);
    uint16_t yOffset = ((line + scrollY) & 7) * 2;
    uint16_t tileMapIndex = scrollX / 8;
    uint16_t tileLine = tileLineAt(tileMapOffset + tileMapIndex, yOffset);
    uint8_t x = scrollX & 7;
    for (uint8_t i = 0; i < 160; i++)
    {
        uint8_t color = colorAt(tileLine, x);
        SDL_SetRenderDrawColor(renderer, color, color, color, 255);
        SDL_RenderDrawPoint(renderer, i, line);
        x++;
        if (x == 8)
        {
            x = 0;
            tileMapIndex++;
            tileLine = tileLineAt(tileMapOffset + tileMapIndex, yOffset);
        }
    }
    SDL_RenderPresent(renderer);
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
            if (line++ < 143)
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
    if (lcdDisplayEnable)
        step(ticks);
    // printf("graphics - clock %d mode %d line %d\n", clock, mode, line);
}

uint8_t Graphics_rb(uint16_t addr)
{
    uint8_t res = 0;
    if (addr < 0xA000)
    {
        addr -= 0x8000;
        res = vram[addr];
    }
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
        case 0xFF41:
            printf("unhandled read from GPU status register");
            exit(1);
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
        case 0xFF47:
            res = palette;
            break;
    }
    // printf("reading from gpu addr %02x val %02x\n", addr, res);
    return res;
}

void Graphics_wb(uint16_t addr, uint8_t val)
{
    if (addr < 0xA000)
    {
        addr -= 0x8000;
        vram[addr] = val;
    }
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
        case 0xFF41:
            printf("unhandled write to GPU status register");
            exit(1);
            break;
        case 0xFF42:
            scrollY = val;
            break;
        case 0xFF43:
            scrollX = val;
            break;
        case 0xFF44:
            // Writing to the LCD Y-Coordinate register (line) resets the counter
            line = 0;
            break;
        case 0xFF47:
            palette = val;
            break;
    }
    // printf("writing to gpu addr %04x val %02x\n", addr, val);
}
