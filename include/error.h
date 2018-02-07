/******************************************************************************
 *
 * file   : error.h
 *
 * author : Tim van Deurzen
 * date   : 17/12/2010
 *
 * Prototypes and macros for the error functions.
 *
 *****************************************************************************/

#ifndef __HIERONYMUS_ERROR_H
#define __HIERONYMUS_ERROR_H

#include <stdio.h>


#ifndef _SUPPRESS_ERRORS
#define HIERONYMUS_ERROR(error, origin) \
    print_error(error, origin)
#else
#define HIERONYMUS_ERROR(error, origin) \
    -errno
#endif

/*
 * Define the actual errors and messages for pretty printing.
 */
#define ERRORS \
    X(err_getattr,          "Could not stat the given path!") \
    X(err_malloc,           "Could not allocate memory!") \
    X(err_getuid,           "Could not acquire user id!") \
    X(err_getpwuid,         "Could not acquire user information!") \
    X(err_mkdir,            "Could not create directory!") \
    X(err_mknod,            "Could not create file node!") \
    X(err_versioning_root,  "Couldn not create versioning root directory!") \
    X(err_readlink,         "Could not read link!") \
    X(err_utime,            "Could not set access / modification time!") \
    X(err_utimens,          "Could not set nanosecond time!") \
    X(err_open,             "Could not open file!") \
    X(err_read,             "Could not read from file!") \
    X(err_statfs,           "Could not statfs the given path!") \
    X(err_create,           "Could not create file!") \
    X(err_ftruncate,        "Could not truncate open file!") \
    X(err_fgetattr,         "Could not fstat the given path!") \
    X(err_opendir,          "Could not open directory!") \
    X(err_readdir,          "Could not read directory!") \
    X(err_rd_filler,        "Readdir buffer full!") \
    X(err_access,           "Could not access path!") \
    X(err_releasedir,       "Could not release directory!") \
    X(err_unlink,           "Could not unlink file!") \
    X(err_rmdir,            "Could not remove directory!") \
    X(err_symlink,          "Could not create symbolic link!") \
    X(err_rename,           "Could not rename file!") \
    X(err_link,             "Could not create hard link!") \
    X(err_chmod,            "Could not change mode bit!") \
    X(err_chown,            "Could not change owner!") \
    X(err_truncate,         "Could not truncate file!") \
    X(err_write,            "Could not write to file!") \
    X(err_release,          "Could not release file!") \
    X(err_setxattr,         "Could not set extended attribute!") \
    X(err_getxattr,         "Could not get extended attribute!") \
    X(err_listxattr,        "Could not list extended attributes!") \
    X(err_removexattr,      "Could not remove extended attribute!") \
    X(err_snapshot,         "Could not find latest snapshot directory!") \
    X(err_system,           "Could not execute system-command!") \
    X(err_vs_write,         "Could not create versioning information!")


/*
 * Define an enumeration of all errors.
 */
#define X(a, b) a,
enum errors { ERRORS };
#undef X


int print_error(int, char *);

char *get_error_message(int);

#endif
