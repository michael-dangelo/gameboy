#include "cpu.h"
#include "graphics.h"
#include "memory.h"

#include <stdint.h>
#include <stdio.h>

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("\nUsage: %s <rom file>\n\n", argv[0]);
        return 1;
    }
    Mem_loadCartridge(argv[1]);
    Graphics_init();
    while (1)
    {
        printf("----------------\n");
        uint8_t ticks = Cpu_step();
        Graphics_step(ticks);
    }
}

