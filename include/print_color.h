/******************************************************************************
 *
 * file   : print_color.h
 * 
 * author : Tim van Deurzen
 * date   : 07/01/2011
 *
 * Macros to enable printing colors in the terminal.
 *
 *****************************************************************************/

#define DEFAULT_BG 27
#define NO_EFFECT 0
#define RED 31
#define GREEN 32
#define BLUE 34

#ifdef _PRINT_COLOR
#define START_PRINT_RED() \
    fprintf(stderr, "%c[%d;%dm", DEFAULT_BG, NO_EFFECT, RED)
#else
#define START_PRINT_RED()
#endif

#ifdef _PRINT_COLOR
#define START_PRINT_GREEN() \
    fprintf(stderr, "%c[%d;%dm", DEFAULT_BG, NO_EFFECT, GREEN)
#else
#define START_PRINT_GREEN()
#endif

#ifdef _PRINT_COLOR
#define START_PRINT_BLUE() \
    fprintf(stderr, "%c[%d;%dm", DEFAULT_BG, NO_EFFECT, BLUE)
#else
#define START_PRINT_BLUE()
#endif

#ifdef _PRINT_COLOR
#define END_PRINT_COLOR() \
    fprintf(stderr, "%c[%dm", DEFAULT_BG, 0);
#else
#define END_PRINT_COLOR()
#endif
