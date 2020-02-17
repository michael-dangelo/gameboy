#include "cpu.h"
#include "graphics.h"
#include "memory.h"

#include "stdio.h"

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("\nUsage: %s <rom file>\n\n", argv[0]);
        return 1;
    }
    Graphics_init();
    Mem_loadCartridge(argv[1]);
    while (1)
    {
        Cpu_step();
        Graphics_step();
    }
}

