#include "cpu.h"
#include "memory.h"

int main()
{
    Mem_loadBootRom();
    for (int i = 0; i < 256; ++i)
        Cpu_step();
    return 0;
}
