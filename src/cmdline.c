/******************************************************************************
 *
 * file   : cmdline.c
 * 
 * author : Tim van Deurzen
 * date   : 06/01/2011
 *
 * Functions for processing commandline arguments and setting various
 * configuration settings.
 *
 *****************************************************************************/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "cmdline.h"
#include "error.h"
#include "util.h"

/** 
 * Parse the commandline arguments.
 *
 * To all intents and purposes there is only one important commandline argument
 * that has to be handled by us instead of by FUSE. That argument is the
 * optional argument specifying the non-standard versioning root directory.
 *
 */
void parse_commandline (int argc, char **argv, char *versioning_root) 
{
    int i = 0;

    /*
     * Look for the `versioning_root' argument.
     */
    for (; i < argc; i++) {
        if (strlen(argv[i]) > 18 
            && strncmp(argv[i], "--versioning_root=", 18) == 0) 
        {
            /* Only copy value (the path) not the `key'. */
            strncpy(versioning_root, argv[i] + 18, strlen(argv[i]) - 18);
            versioning_root[(strlen(argv[i]) - 18)] = '\0';

            break;
        }
    }
    
    /*
     * Fix the array of arguments, removing the 
     * `versioning_root' argument.
     */
    for (; i < argc; i++) {
        argv[i] = argv[i + 1];
    }
}

/** 
 * Add arguments to the commandline arguments.
 *
 * This function extends the set of commandline arguments with a new argument.
 * This way special arguments can be passed to FUSE, for instance:
 *
 *     ``-o nonempty''
 *
 * NOTE: this function is mostly a luxury, the arguments can very simply be
 * specified on the commandline. Also this function introduces a memory leak by
 * not freeing up / reusing the memory used by the original argv. Realloc won't
 * work here as the argv was not malloc'ed.
 *
 * TODO: free argv structure, conserving the pointer.
 */
int add_commandline_arg(int argc, char ***argv, char *new_arg) 
{
    char **new_argv = NULL;
    int i = 0;

    /* 
     * Allocate memory for the new argument array. 
     */
    new_argv = (char **) checked_malloc((argc + 2) * sizeof(char *));

    if (new_argv == NULL) {
        HIERONYMUS_ERROR(err_malloc, "add_commandline_arg");
        abort();
    }

    for (; i < argc; i++) {
        new_argv[i] = (* argv)[i];
    }

    new_argv[argc] = (char *) checked_malloc(MAX_ARG_LENGTH * sizeof(char));
    new_argv[argc + 1] = (char *) checked_malloc(MAX_ARG_LENGTH * sizeof(char));
    
    if (strncmp(new_arg, "-o", 2) == 0) {
        /* 
         * Here we actually have two arguments: '-o', 'nonempty'. 
         */
        strncpy(new_argv[argc], "-o", 2);
        strncpy(new_argv[argc + 1], new_arg + 3, strlen(new_arg) - 3);
        argc += 2;

    } else {
        /*
         * Now we just have a single argument: '-f'. 
         */
        strncpy(new_argv[argc], new_arg, strlen(new_arg));
        argc += 1;
    }
    
    *argv = new_argv;

    return argc;
}
