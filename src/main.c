#include "cartridge.h"
#include "cpu.h"
#include "graphics.h"
#include "memory.h"
#include "sound.h"
#include "timer.h"

#include <stdint.h>
#include <stdio.h>

uint8_t enableDebugPrints = 1;

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("\nUsage: %s <rom file>\n\n", argv[0]);
        return 1;
    }

    Cpu_init();
    Cartridge_load(argv[1]);
    Graphics_init();
    Memory_init();
    Sound_init();

    while (1)
    {
        uint8_t ticks = Cpu_step();
        Graphics_step(ticks);
        Timer_step(ticks);
        Sound_step(ticks);
        Cpu_interrupts();
    }
}

