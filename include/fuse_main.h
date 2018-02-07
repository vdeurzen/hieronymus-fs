/******************************************************************************
 *
 * file   : fuse_main.h
 *
 * author : Tim van Deurzen
 * date   : 27/01/2011
 *
 * Prototypes and macros for use within the Fuse functions and the main
 * function.
 *
 *****************************************************************************/

#ifndef __HIERONYMUS_INTERFACE_H
#define __HIERONYMUS_INTERFACE_H


typedef struct HIERONYMUS_DATA {
    char *root_directory;
    int max_num_versions;
    FILE *log_file;
} hieronymus_data;

/*
 * Access the private data field of the FUSE context.
 */
#define ADMIN ((hieronymus_data *) fuse_get_context()->private_data)

/*
 * Access the process information in the the FUSE context.
 */
#define OP_PID fuse_get_context()->pid
#define OP_GID fuse_get_context()->gid
#define OP_UID fuse_get_context()->uid


#define MAX_NUM_VERSIONS 16


void resolve_root_path(const char *, char *);

#endif
