/******************************************************************************
 *
 * file   : util.h
 *
 * author : Tim van Deurzen
 * date   : 06/01/2011
 *
 * Prototypes and macros for all the utility functions.
 *
 *****************************************************************************/

#ifndef __HIERONYMUS_UTIL_H
#define __HIERONYMUS_UTIL_H

#include <stdio.h>


#define SHA1_LENGTH 20
#define MAX_ID_LENGTH 128
#define MAX_SNAPSHOT_LENGTH 128
#define MAX_COMMAND_LENGTH 512

#ifdef _DEBUG
#define HIERONYMUS_DEBUG(format, ...) \
    start_print_debug(); \
    fprintf(stderr, format, __VA_ARGS__); \
    end_print_debug()

#define HIERONYMUS_NOTE(str) \
    start_print_debug(); \
    fprintf(stderr, str); \
    end_print_debug()
#else
#define HIERONYMUS_DEBUG(format, ...)
#define HIERONYMUS_NOTE(str)
#endif


int create_versioning_root(char *, char *);

void sha1_str(const char *, char *);

int checked_mkdir(const char *);

int make_snapshot_directory(const char *, char *);

int find_latest_snapshot(const char *, char*);

int find_snapshot_version(const char *, const char *);

void *checked_malloc(int);

void start_print_debug(void);

void end_print_debug(void);

void parent_directory(const char *, char *);

void bottom_directory(const char *, char *);

char *timestamp(void);

int copy(const char *, const char *);

int diff(const char *, const char *);

#endif
