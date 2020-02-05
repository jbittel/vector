/*

  ----------------------------------------------------
  vsm - vector space model data similarity
  ----------------------------------------------------

  Copyright (c) 2008 Jason Bittel <jason.bittel@gmail.com>

*/

#ifndef _HAVE_ERROR_H
#define _HAVE_ERROR_H

#include <signal.h>

extern int quiet_mode;

/* Macros for logging/displaying status messages */
#define PRINT(x...) { if (!quiet_mode) { fprintf(stderr, x); fprintf(stderr, "\n"); } }
#define WARN(x...) { fprintf(stderr, "Warning: " x); fprintf(stderr, "\n"); }
#define DIE(x...) { fprintf(stderr, "Error: " x); fprintf(stderr, "\n"); raise(SIGINT); }

/* Assert macro for testing and debugging; use 'make debug'
   to compile the program with debugging features enabled */
#ifdef DEBUG
#define ASSERT(x)                                                    \
        if (!(x)) {                                                  \
                fflush(NULL);                                        \
                fprintf(stderr, "\nAssertion failed: %s, line %d\n", \
                                __FILE__, __LINE__);                 \
                fflush(stderr);                                      \
                exit(1);                                             \
        }
#endif

#endif /* ! _HAVE_ERROR_H */
