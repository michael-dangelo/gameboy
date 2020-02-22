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

void Graphics_step(void)
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
        if (event.type == SDL_QUIT)
            exit(0);
    SDL_Delay(10);
}
