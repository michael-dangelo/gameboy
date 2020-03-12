#include "graphics.h"

#include "cartridge.h"
#include "debug.h"
#include "input.h"

#include "SDL/SDL.h"

#include "windows.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static SDL_Window *window = NULL;
static SDL_Surface *screen = NULL;
static SDL_Renderer *renderer = NULL;

#ifdef DEBUG_TILES
static SDL_Window *debug_window = NULL;
static SDL_Surface *debug_screen = NULL;
static SDL_Renderer *debug_renderer = NULL;
void drawDebugTiles(void);
#endif

static uint8_t vram[0x2000];
static uint8_t oam[0xA0];
static uint16_t clock = 0;

#define WIDTH 160
#define HEIGHT 144
static uint8_t s_fps = 60;
static uint8_t s_scale = 3;
static LARGE_INTEGER freq;

#define NUM_COLORS 4
static uint8_t s_colors[NUM_COLORS] = {220, 192, 96, 0};

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

// FF42 - BG Scroll Y
static uint8_t bgScrollY = 0;

// FF43 - BG Scroll X
static uint8_t bgScrollX = 0;

// FF44 - LDC Y-Coordinate
static uint8_t line = 0;

// FF45 - LY Compare
static uint8_t lineCompare = 0;

// FF47 - BG Palette Data
static uint8_t bgPalette = 0;

// FF48 - Object Palette 0
static uint8_t objPalette0 = 0;

// FF49 - Object Palette 1
static uint8_t objPalette1 = 0;

// FF4A - Window Scroll Y
static uint8_t windowScrollY;

// FF4B - Window Scroll X
static uint8_t windowScrollX;

// V-Blank Interrupt Request
static uint8_t vblankInterruptRequest = 0;

// Status Interrupt Request
static uint8_t statusInterruptRequest = 0;

void cleanup(void)
{
    Cartridge_writeSaveFile();
    SDL_Quit();
}

void Graphics_init(void)
{
    if (SDL_Init(SDL_INIT_VIDEO))
    {
        printf("Failed to initialize SDL\n");
        exit(1);
    }
    atexit(cleanup);
    window = SDL_CreateWindow(
        "Gameboy", 500, 250,  WIDTH * s_scale, HEIGHT * s_scale, 0
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
#ifdef DEBUG_TILES
    debug_window = SDL_CreateWindow("Debug Tilemap", 700, 300, 256 * s_scale, 256 * s_scale, 0);
    debug_screen = SDL_GetWindowSurface(debug_window);
    debug_renderer = SDL_CreateRenderer(debug_window, -1, 0);
    if (!debug_window || !debug_screen || !debug_renderer)
    {
        printf("Something went wrong trying to debug graphics\n");
        exit(1);
    }
    SDL_RenderSetScale(debug_renderer, s_scale, s_scale);
#endif
    SDL_RenderSetScale(renderer, s_scale, s_scale);
    QueryPerformanceFrequency(&freq);
}

#ifndef DISABLE_RENDER
static uint16_t tileLineAt(uint16_t addr, uint16_t yOffset)
{
    uint16_t tile = tileDataSelect ? (uint16_t)vram[addr] * 16
                                   : 0x1000 + ((int8_t)vram[addr] * 16);
    uint16_t pixels = tile + yOffset;
    return vram[pixels] + ((uint16_t)vram[pixels + 1] << 8);
}

static uint8_t colorIndexAt(uint16_t tileLine, uint8_t x)
{
    uint8_t l = (tileLine >> 8) & 0xFF;
    uint8_t h = tileLine & 0xFF;
    uint8_t pixelOffset = 7 - x; // leftmost pixel is 7th bit
    return ((l >> pixelOffset) & 1) | (((h >> pixelOffset) & 1) << 1);
}

static uint8_t colorAt(uint16_t tileLine, uint8_t x, uint8_t palette)
{
    uint8_t colorIndex = colorIndexAt(tileLine, x);
    uint8_t color = (palette >> (colorIndex * 2)) & 3;
    return color;
}

static void renderBg(
    int colorIndex[NUM_COLORS],
    SDL_Point colorPoints[NUM_COLORS][WIDTH])
{
    uint16_t bgMap = bgTileMapSelect ? 0x1C00 : 0x1800;
    uint16_t tileMapOffset = bgMap + (((uint8_t)(line + bgScrollY) / 8) * 32);
    uint16_t tileMapRowEnd = tileMapOffset + 32;
    uint16_t wrappedLine = line + bgScrollY;
    if (wrappedLine >= HEIGHT)
        wrappedLine -= HEIGHT;
    uint16_t yOffset = (wrappedLine & 7) * 2;
    uint16_t tileMapIndex = bgScrollX / 8;
    uint16_t tileMapAddr = tileMapOffset + tileMapIndex;
    if (tileMapAddr >= tileMapRowEnd)
        tileMapAddr -= 32;
    uint16_t tileLine = tileLineAt(tileMapAddr, yOffset);
    uint8_t x = bgScrollX & 7;
    for (uint8_t i = 0; i < WIDTH; i++)
    {
        uint8_t color = colorAt(tileLine, x, bgPalette);
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
            tileMapAddr = tileMapOffset + tileMapIndex;
            if (tileMapAddr >= tileMapRowEnd)
                tileMapAddr -= 32;
            tileLine = tileLineAt(tileMapAddr, yOffset);
        }
    }
}

static void renderWindow(
    int colorIndex[NUM_COLORS],
    SDL_Point colorPoints[NUM_COLORS][WIDTH])
{
    uint16_t windowMap = windowTileMapSelect ? 0x1C00 : 0x1800;
    uint16_t tileMapOffset = windowMap + ((line / 8) * 32);
    uint16_t yOffset = (line & 7) * 2;
    uint16_t tileMapIndex = 0;
    uint16_t tileMapAddr = tileMapOffset + tileMapIndex;
    uint16_t tileLine = tileLineAt(tileMapAddr, yOffset);
    uint8_t x = 0;
    for (uint8_t i = 0; i < WIDTH; i++)
    {
        uint8_t color = colorAt(tileLine, x, bgPalette);
        SDL_Point p;
        p.x = i + windowScrollX - 7;
        p.y = line + windowScrollY;
        colorPoints[color][colorIndex[color]] = p;
        colorIndex[color]++;
        x++;
        if (x == 8)
        {
            x = 0;
            tileMapIndex++;
            tileMapAddr = tileMapOffset + tileMapIndex;
            tileLine = tileLineAt(tileMapAddr, yOffset);
        }
    }
}

static void renderSprites(
    int colorIndex[2][NUM_COLORS],
    SDL_Point colorPoints[2][NUM_COLORS][WIDTH])
{
    if (!spriteDisplayEnable)
        return;

    for (uint16_t i = 0; i < 0xA0; i += 0x4)
    {
        uint8_t spriteY = oam[i] - 16;
        uint8_t spriteX = oam[i + 1] - 8;
        uint16_t tile = oam[i + 2] * 16;
        uint8_t flags = oam[i + 3];

        if (line < spriteY || spriteY + 7 < line)
            continue;

        uint8_t palette = (flags >> 4) & 1 ? objPalette1 : objPalette0;
        uint8_t flipX = (flags >> 5) & 1;
        uint8_t flipY = (flags >> 6) & 1;
        uint8_t priority = (flags >> 7) & 1;

        uint8_t y = line - spriteY;
        uint16_t pixelsAddr = tile + ((flipY ? 7 - y : y) * 2);
        uint16_t pixels = vram[pixelsAddr] + ((uint16_t)vram[pixelsAddr + 1] << 8);
        for (uint8_t x = 0; x < 8; x++)
        {
            if (!colorIndexAt(pixels, flipX ? 7 - x : x))
                continue;
            uint8_t color = colorAt(pixels, flipX ? 7 - x : x, palette);
            SDL_Point p;
            p.x = spriteX + x;
            p.y = spriteY + y;
            colorPoints[priority][color][colorIndex[priority][color]] = p;
            colorIndex[priority][color]++;
        }
    }
}

static void renderPoints(uint8_t color, SDL_Point points[], int count)
{
    SDL_SetRenderDrawColor(renderer, color, color, color, 255);
    SDL_RenderDrawPoints(renderer, points, count);
}

static void renderScanline(void)
{
    int bgIndex[NUM_COLORS] = {0};
    int windowIndex[NUM_COLORS] = {0};
    int spriteIndex[2][NUM_COLORS] = {0};
    SDL_Point bgPoints[NUM_COLORS][WIDTH] = {0};
    SDL_Point windowPoints[NUM_COLORS][WIDTH] = {0};
    SDL_Point spritePoints[2][NUM_COLORS][WIDTH] = {0};

    renderBg(bgIndex, bgPoints);
    if (windowDisplayEnable)
        renderWindow(windowIndex, windowPoints);
    renderSprites(spriteIndex, spritePoints);

    uint8_t bgColor0 = bgPalette & 3;
    renderPoints(s_colors[bgColor0], bgPoints[bgColor0], bgIndex[bgColor0]);
    renderPoints(s_colors[bgColor0], windowPoints[bgColor0], windowIndex[bgColor0]);
    for (uint8_t i = 0; i < NUM_COLORS; i++)
        renderPoints(s_colors[i], spritePoints[1][i], spriteIndex[1][i]);
    for (uint8_t i = 0; i < NUM_COLORS; i++)
    {
        if (i == bgColor0)
            continue;
        renderPoints(s_colors[i], bgPoints[i], bgIndex[i]);
        renderPoints(s_colors[i], windowPoints[i], windowIndex[i]);
    }
    for (uint8_t i = 0; i < NUM_COLORS; i++)
        renderPoints(s_colors[i], spritePoints[0][i], spriteIndex[0][i]);
#ifdef DEBUG_TILES
    drawDebugTiles();
#endif
}
#endif

void render(void)
{
    SDL_RenderPresent(renderer);
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch(event.type)
        {
            case SDL_QUIT:
                exit(0);
            case SDL_KEYDOWN:
            case SDL_KEYUP:
                Input_pressed(&event);
                break;
        }
    }

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
                if (oamInterruptEnable)
                {
                    INT_PRINT(("graphics requesting oam status interrupt\n"));
                    statusInterruptRequest = 1;
                }
            }
            else
            {
                mode = VBLANK;
                INT_PRINT(("graphics requesting vblank interrupt\n"));
                vblankInterruptRequest = 1;
                if (vblankInterruptEnable)
                    statusInterruptRequest = 1;
            }
            lineCompareFlag = line == lineCompare;
            if (lineCompareInterruptEnable && lineCompareFlag)
            {
                INT_PRINT(("graphics requesting line compare status interrupt\n"));
                statusInterruptRequest = 1;
            }
            break;
        case VBLANK:
            if (clock < 456)
                return;
            clock = 0;
            line++;
            lineCompareFlag = line == lineCompare;
            if (lineCompareInterruptEnable && lineCompareFlag)
            {
                INT_PRINT(("graphics requesting line compare status interrupt\n"));
                statusInterruptRequest = 1;
            }
            if (line <= 153)
                return;
            line = 0;
            lineCompareFlag = line == lineCompare;
            if (lineCompareInterruptEnable && lineCompareFlag)
            {
                INT_PRINT(("graphics requesting line compare status interrupt\n"));
                statusInterruptRequest = 1;
            }
            mode = OAM;
            if (oamInterruptEnable)
            {
                INT_PRINT(("graphics requesting oam status interrupt\n"));
                statusInterruptRequest = 1;
            }
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
            if (hblankInterruptEnable)
            {
                INT_PRINT(("graphics requesting hblank interrupt\n"));
                statusInterruptRequest = 1;
            }
            break;
    }
    return;
}

#ifdef DEBUG_TILES
static uint16_t frameCount = 0;
void drawDebugTiles(void)
{
    // only draw a few frames, expensive
    frameCount++;
    static uint16_t framesPerRender = 100;
    if (frameCount % framesPerRender)
        return;

    uint16_t tileMap = bgTileMapSelect ? 0x1C00 : 0x1800;
    for (uint16_t line_ = 0; line_ < 256; line_++)
    {
        int colorIndex[NUM_COLORS] = {0};
        SDL_Point colorPoints[NUM_COLORS][256] = {0};
        uint16_t tileMapOffset = tileMap + ((line_ / 8) * 32);
        uint16_t yOffset = (line_ & 7) * 2;
        uint16_t tileMapIndex = 0;
        GPU_PRINT(("tile map at %04x pixels %04x\n", tileMapOffset + tileMapIndex + 0x8000, tileLine));
        uint16_t tileLine = tileLineAt(tileMapOffset + tileMapIndex, yOffset);
        uint8_t x = 0;
        for (uint16_t i = 0; i < 256; i++)
        {
            uint8_t color = colorAt(tileLine, x, bgPalette);
            SDL_Point p;
            p.x = i;
            p.y = line_;
            colorPoints[color][colorIndex[color]] = p;
            colorIndex[color]++;
            x++;
            if (x == 8)
            {
                x = 0;
                tileMapIndex++;
                GPU_PRINT(("tile map at %04x pixels %04x\n", tileMapOffset + tileMapIndex + 0x8000, tileLine));
                tileLine = tileLineAt(tileMapOffset + tileMapIndex, yOffset);
            }
        }
        for (uint8_t i = 0; i < NUM_COLORS; i++)
        {
            uint8_t color = s_colors[i];
            SDL_SetRenderDrawColor(debug_renderer, color, color, color, 255);
            SDL_Point *points = colorPoints[i];
            int count = colorIndex[i];
            SDL_RenderDrawPoints(debug_renderer, points, count);
        }
    }
    SDL_SetRenderDrawColor(debug_renderer, 0xAA, 0x33, 0x66, 255);
    SDL_RenderDrawLine(debug_renderer, bgScrollX, bgScrollY, bgScrollX + WIDTH, bgScrollY);
    SDL_RenderDrawLine(debug_renderer, bgScrollX + WIDTH, bgScrollY, bgScrollX + WIDTH, bgScrollY + HEIGHT);
    SDL_RenderDrawLine(debug_renderer, bgScrollX + WIDTH, bgScrollY + HEIGHT, bgScrollX, bgScrollY + HEIGHT);
    SDL_RenderDrawLine(debug_renderer, bgScrollX, bgScrollY + HEIGHT, bgScrollX, bgScrollY);
    SDL_RenderPresent(debug_renderer);
}
#endif

void Graphics_step(uint8_t ticks)
{
    if (lcdDisplayEnable)
        step(ticks);
    GPU_PRINT(("graphics mode %02x line %02x\n", mode, line));
}

uint8_t Graphics_rb(uint16_t addr)
{
    uint8_t res = 0;
    if (addr < 0xA000)
        res = vram[addr - 0x8000];
    else if (addr < 0xFEA0)
        res = oam[addr - 0xFE00];

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
            GPU_PRINT(("gpu read control, val %02x\n", res));
            break;
        case 0xFF41:
            res = (lineCompareInterruptEnable << 6) |
                  (oamInterruptEnable << 5) |
                  (vblankInterruptEnable << 4) |
                  (hblankInterruptEnable << 3) |
                  (lineCompareFlag << 2) |
                  (mode & 3);
            GPU_PRINT(("gpu read status, val %02x\n", res));
            break;
        case 0xFF42:
            res = bgScrollY;
            GPU_PRINT(("gpu read bg scrollY, val %02x\n", res));
            break;
        case 0xFF43:
            res = bgScrollX;
            GPU_PRINT(("gpu read bg scrollX, val %02x\n", res));
            break;
        case 0xFF44:
            res = line;
            GPU_PRINT(("gpu read line, val %02x\n", res));
            break;
        case 0xFF45:
            res = lineCompare;
            GPU_PRINT(("gpu read lineCompare, val %02x\n", res));
            break;
        case 0xFF47:
            res = bgPalette;
            GPU_PRINT(("gpu read bg palette, val %02x\n", res));
            break;
        case 0xFF48:
            res = objPalette0;
            GPU_PRINT(("gpu read obj palette 0, val %02x\n", res));
            break;
        case 0xFF49:
            res = objPalette1;
            GPU_PRINT(("gpu read obj palette 1, val %02x\n", res));
            break;
        case 0xFF4A:
            res = windowScrollY;
            GPU_PRINT(("gpu read window scrollY, val %02x\n", res));
            break;
        case 0xFF4B:
            res = windowScrollX;
            GPU_PRINT(("gpu read window scrollX, val %02x\n", res));
            break;
    }
    return res;
}

void Graphics_wb(uint16_t addr, uint8_t val)
{
    if (addr < 0xA000)
        vram[addr - 0x8000] = val;
    else if (addr < 0xFEA0)
        oam[addr - 0xFE00] = val;

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
            GPU_PRINT(("gpu write lcd control, val %02x\n", val));
            break;
        case 0xFF41:
            lineCompareInterruptEnable = (val >> 6) & 1;
            oamInterruptEnable = (val >> 5) & 1;
            vblankInterruptEnable = (val >> 4) & 1;
            hblankInterruptEnable = (val >> 3) & 1;
            GPU_PRINT(("gpu write lcd status, val %02x\n", val));
            break;
        case 0xFF42:
            bgScrollY = val;
            GPU_PRINT(("gpu write bg scrollY, val %02x\n", val));
            break;
        case 0xFF43:
            bgScrollX = val;
            GPU_PRINT(("gpu write bg scrollX, val %02x\n", val));
            break;
        case 0xFF44:
            line = 0;
            GPU_PRINT(("gpu write line, val %02x\n", val));
            break;
        case 0xFF45:
            lineCompare = val;
            GPU_PRINT(("gpu write lineCompare, val %02x\n", val));
            break;
        case 0xFF47:
            bgPalette = val;
            GPU_PRINT(("gpu write bg palette, val %02x\n", val));
            break;
        case 0xFF48:
            objPalette0 = val;
            GPU_PRINT(("gpu write obj palette 0, val %02x\n", val));
            break;
        case 0xFF49:
            objPalette1 = val;
            GPU_PRINT(("gpu write obj palette 1, val %02x\n", val));
            break;
        case 0xFF4A:
            windowScrollY = val;
            GPU_PRINT(("gpu write window scrollY, val %02x\n", val));
            break;
        case 0xFF4B:
            windowScrollX = val;
            GPU_PRINT(("gpu write window scrollX, val %02x\n", val));
            break;
    }
}

uint8_t Graphics_vblankInterrupt(void)
{
    uint8_t interrupt = vblankInterruptRequest;
    vblankInterruptRequest = 0;
    return interrupt;
}

uint8_t Graphics_statusInterrupt(void)
{
    uint8_t interrupt = statusInterruptRequest;
    statusInterruptRequest = 0;
    return interrupt;
}

void Graphics_dma(const uint8_t *dmaAddress)
{
    GPU_PRINT(("graphics dma copy from address %p", dmaAddress));
    memcpy(oam, dmaAddress, 0xA0);
}
