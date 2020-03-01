#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>

extern uint8_t enableDebugPrints;

#define DEBUG
#define DEBUG_CPU
#define DEBUG_MEMORY
#define DEBUG_GRAPHICS

#define DISABLE_RENDER

#ifdef DEBUG
#define PRINT(x) if (enableDebugPrints) printf x
#else
#define PRINT(x)
#endif

#ifdef DEBUG_CPU
#define CPU_PRINT(x) PRINT(x)
#else
#define CPU_PRINT(x)
#endif

#ifdef DEBUG_MEMORY
#define MEM_PRINT(x) PRINT(x)
#else
#define MEM_PRINT(x)
#endif

#ifdef DEBUG_GRAPHICS
#define GPU_PRINT(x) PRINT(x)
#else
#define GPU_PRINT(x)
#endif

#endif
