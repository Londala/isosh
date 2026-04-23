#ifndef MACROS_H
#define MACROS_H

#include <stdio.h>

#include "ANSI-color-codes.h"


// DEBUG print macro, set DEBUG to 0 to disable
#ifndef DEBUG
    #define DEBUG 0
#endif
#if defined(DEBUG) && DEBUG > 0
 #define DEBUG_PRINT(fmt, args...) fprintf(stderr, BRED "DEBUG: " RED "%s:%d:%s(): " fmt CRESET, \
    __FILE__, __LINE__, __func__, ##args)
#else
 #define DEBUG_PRINT(fmt, args...) /* Don't do anything in release builds */
#endif

// clear the terminal
#define clear() printf("\033[H\033[J")

#endif
