/******************************************************************************
 *
 * file   : error.h
 *
 * author : Tim van Deurzen
 * date   : 06/01/2011
 *
 * Functions for pretty printing errors.
 *
 *****************************************************************************/

#include <errno.h>
#include <string.h>

#include "print_color.h"
#include "error.h"

/*
 * Define a string-array for pretty-printing errors.
 */
#define X(a, b) b,
static char *error_messages[] = { ERRORS };
#undef X

/**
 * Print an error message.
 *
 * Errors are printed in red to make them stand out in the debug output.
 */
int print_error(int error, char *origin)
{
    START_PRINT_RED();

    fprintf(stderr, "*** ERROR | %s\n", get_error_message(error));
    fprintf(stderr, "%9s | %s: %s\n", "", origin, strerror(errno));

    END_PRINT_COLOR();

    return -errno;
}

/**
 * Return the error message belonging to the given error number.
 */
char *get_error_message(int error_number) 
{
    return error_messages[error_number];
}
