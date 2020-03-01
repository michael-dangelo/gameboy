#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>

// #define DEBUG

#ifdef DEBUG
#define PRINT(x) printf x
#else
#define PRINT(x) do {} while (0)
#endif

#endif
