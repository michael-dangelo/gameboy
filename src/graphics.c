#include "graphics.h"

#include "debug.h"

#include "SDL/SDL.h"

#include "windows.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static SDL_Window *window = NULL;
static SDL_Surface *screen = NULL;
static SDL_Renderer *renderer = NULL;

static uint8_t vram[0x2000];
static uint16_t clock = 0;

static uint8_t s_fps = 60;
static uint8_t s_scale = 3;
static LARGE_INTEGER freq;

typedef enum {
    HBLANK,
    VBLANK,
    OAM,
    VRAM
} Mode;

// FF40 - LCD control
static uint8_t lcdDisplayEnable = 0;
static uint8_t windowTileMapSelect = 0;
static uint8_t windowDisplayEnable = 0;
static uint8_t tileDataSelect = 0;
static uint8_t bgTileMapSelect = 0;
static uint8_t spriteSize = 0;
static uint8_t spriteDisplayEnable = 0;
static uint8_t bgDisplay = 0;
static Mode mode = OAM;

// FF41 - LCD status
static uint8_t lineCompareInterruptEnable = 0;
static uint8_t oamInterruptEnable = 0;
static uint8_t vblankInterruptEnable = 0;
static uint8_t hblankInterruptEnable = 0;
static uint8_t lineCompareFlag = 0;

// FF42 - Scroll Y
static uint8_t scrollY = 0;

// FF43 - Scroll X
static uint8_t scrollX = 0;

// FF44 - LDC Y-Coordinate
static uint8_t line = 0;

// FF47 - BG Palette Data
static uint8_t palette = 0;

// V-Blank Interrupt Request
static uint8_t vblankInterruptRequest = 0;

void Graphics_init(void)
{
    if (SDL_Init(SDL_INIT_VIDEO))
    {
        printf("Failed to initialize SDL\n");
        exit(1);
    }
    atexit(SDL_Quit);
    window = SDL_CreateWindow(
        "Gameboy", 500, 250,  160 * s_scale, 144 * s_scale, 0
    );
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
    SDL_RenderSetScale(renderer, s_scale, s_scale);
    QueryPerformanceFrequency(&freq);
}

#ifndef DISABLE_RENDER
static uint16_t tileLineAt(uint16_t addr, uint16_t yOffset)
{
    uint16_t tile = tileDataSelect ? (uint16_t)vram[addr] * 16
                                   : 0x1000 + ((int16_t)vram[addr] * 16);
    GPU_PRINT(("tile at %04x\n", tile + 0x8000));
    uint16_t pixels = tile + yOffset;
    GPU_PRINT(("adding yOffset %04x to get pixels at %04x\n", yOffset, pixels + 0x8000));
    return vram[pixels] + ((uint16_t)vram[pixels + 1] << 8);
}

static uint8_t colorAt(uint16_t tileLine, uint8_t x)
{
    uint8_t l = (tileLine >> 8) & 0xFF;
    uint8_t h = tileLine & 0xFF;
    uint8_t pixelOffset = 7 - x; // leftmost pixel is 7th bit
    uint8_t colorIndex = ((l >> pixelOffset) & 1) | (((h >> pixelOffset) & 1) << 1);
    uint8_t color = (palette >> (colorIndex * 2)) & 3;
    GPU_PRINT(("x %02x offset %02x colorIndex %02x palettedColor %02x\n",
        x, pixelOffset, colorIndex, color));
    return color;
}

static void renderScanline()
{
    int colorIndex[4] = {0};
    SDL_Point colorPoints[4][160] = {0};
    uint16_t tileMap = bgTileMapSelect ? 0x1C00 : 0x1800;
    GPU_PRINT(("line %02x scroll %02x line + scrollY %02x\n", line, scrollY, line + scrollY));
    uint16_t tileMapOffset = tileMap + (((uint8_t)(line + scrollY) / 8) * 32);
    uint16_t yOffset = ((line + scrollY) & 7) * 2;
    uint16_t tileMapIndex = scrollX / 8;
    uint16_t tileLine = tileLineAt(tileMapOffset + tileMapIndex, yOffset);
    GPU_PRINT(("tile map at %04x pixels %04x\n", tileMapOffset + tileMapIndex + 0x8000, tileLine));
    uint8_t x = scrollX & 7;
    for (uint8_t i = 0; i < 160; i++)
    {
        uint8_t color = colorAt(tileLine, x);
        SDL_Point p;
        p.x = i;
        p.y = line;
        colorPoints[color][colorIndex[color]] = p;
        colorIndex[color]++;
        x++;
        if (x == 8)
        {
            x = 0;
            tileMapIndex++;
            tileLine = tileLineAt(tileMapOffset + tileMapIndex, yOffset);
            GPU_PRINT(("tile map at %04x pixels %04x\n", tileMapOffset + tileMapIndex + 0x8000, tileLine));
        }
    }
    for (uint8_t i = 0; i < 4; i++)
    {
        static uint8_t colors[4] = {220, 192, 96, 0};
        uint8_t color = colors[i];
        SDL_SetRenderDrawColor(renderer, color, color, color, 255);
        SDL_Point *points = colorPoints[i];
        int count = colorIndex[i];
        SDL_RenderDrawPoints(renderer, points, count);
    }
}
#endif

void render(void)
{
    SDL_RenderPresent(renderer);
    SDL_Event event;
    while (SDL_PollEvent(&event))
        if (event.type == SDL_QUIT)
            exit(0);

    static LARGE_INTEGER start, end;
    static uint8_t started = 0;
    const double msPerFrame = 1000.0 / s_fps;
    if (started)
    {
        QueryPerformanceCounter(&end);
        double elapsed = (end.QuadPart - start.QuadPart) * 1000.0 / freq.QuadPart;
        double remaining = msPerFrame - elapsed;
        if (remaining > 5)
            SDL_Delay((uint32_t)remaining);
        QueryPerformanceCounter(&end);
    }
    started = 1;
    QueryPerformanceCounter(&start);
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
            {
                mode = OAM;
            }
            else
            {
                mode = VBLANK;
                if (vblankInterruptEnable)
                    vblankInterruptRequest = 1;
            }
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
            render();
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
#ifndef DISABLE_RENDER
            renderScanline();
#endif
            break;
    }
    return;
}

void Graphics_step(uint8_t ticks)
{
    if (lcdDisplayEnable)
        step(ticks);
}

uint8_t Graphics_rb(uint16_t addr)
{
    uint8_t res = 0;
    if (addr < 0xA000)
    {
        res = vram[addr - 0x8000];
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
            res = (lineCompareInterruptEnable << 6) |
                  (oamInterruptEnable << 5) |
                  (vblankInterruptEnable << 4) |
                  (hblankInterruptEnable << 3) |
                  (lineCompareFlag << 2) |
                  (mode & 3);
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
    MEM_PRINT(("reading from gpu addr %04x val %02x\n", addr, res));
    return res;
}

void Graphics_wb(uint16_t addr, uint8_t val)
{
    if (addr < 0xA000)
        vram[addr - 0x8000] = val;

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
            lineCompareInterruptEnable = (val >> 6) & 1;
            oamInterruptEnable = (val >> 5) & 1;
            vblankInterruptEnable = (val >> 4) & 1;
            hblankInterruptEnable = (val >> 3) & 1;
            break;
        case 0xFF42:
            scrollY = val;
            break;
        case 0xFF43:
            scrollX = val;
            break;
        case 0xFF44:
            line = 0;
            break;
        case 0xFF47:
            palette = val;
            break;
    }
    MEM_PRINT(("writing to gpu addr %04x val %02x\n", addr, val));
}

uint8_t Graphics_vblankInterrupt(void)
{
    uint8_t interrupt = vblankInterruptRequest;
    vblankInterruptRequest = 0;
    return interrupt;
}
