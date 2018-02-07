/******************************************************************************
 *
 * file   : log.c
 *
 * author : Tim van Deurzen
 * date   : 27/01/2011
 *
 * Contains all functionality needed for logging purposes.
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "log.h"

/**
 * Open the log file for writing.
 */
FILE *open_log_file (void)
{
    FILE *file_handle;

    file_handle = fopen("./hieronymus.log", "w");

    if (file_handle == NULL) {
        perror("open_log_file");
        exit(EXIT_FAILURE);
    }

    /* Set file to line buffering */
    setvbuf(file_handle, NULL, _IOLBF, 0);

    return file_handle;
}
