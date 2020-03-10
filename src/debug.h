#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>

extern uint8_t enableDebugPrints;

// #define DEBUG
// #define DEBUG_CPU
// #define DEBUG_MEMORY
// #define DEBUG_GRAPHICS
// #define DEBUG_TIMER
#define DEBUG_INPUT
#define DEBUG_INTERRUPTS
// #define DISABLE_RENDER
#define SKIP_BOOTROM

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

#ifdef DEBUG_TIMER
#define TIMER_PRINT(x) PRINT(x)
#else
#define TIMER_PRINT(x)
#endif

#ifdef DEBUG_INPUT
#define INPUT_PRINT(x) PRINT(x)
#else
#define INPUT_PRINT(x)
#endif

#ifdef DEBUG_INTERRUPTS
#define INT_PRINT(x) PRINT(x)
#else
#define INT_PRINT(x)
#endif

#endif
