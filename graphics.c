#include "graphics.h"

#include "SDL/SDL.h"

#include <stdlib.h>

void Graphics_init(void)
{
    SDL_Init(SDL_INIT_VIDEO);
    atexit(SDL_Quit);
}

void Graphics_step(void)
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
        if (event.type == SDL_QUIT)
            exit(0);
}

