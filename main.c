#include "cpu.h"
#include "memory.h"

int main()
{
    loadBootRom();
    for (int i = 0; i < 256; ++i)
        cpuStep();
    return 0;
}
