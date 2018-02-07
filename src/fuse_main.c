/******************************************************************************
 *
 * file   : fuse_main.c
 *
 * author : Tim van Deurzen
 * date   : 14/12/2010
 *
 *  ** Start writing article.
 *  ** Clean up Makefile.
 *
 *  ***** Planned *****
 *  - Add versioning for symbolic links.
 *****************************************************************************/

#define FUSE_USE_VERSION  26

#ifdef linux
/* For pread() / pwrite(). */
#define _XOPEN_SOURCE 500
#endif

#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <limits.h>
#include <pwd.h>
#include <errno.h>
#include <fcntl.h>
#include <utime.h>
#include <sys/types.h>
#include <sys/xattr.h>
#include <sys/time.h>

#include "fuse_main.h"
#include "util.h"
#include "error.h"
#include "cmdline.h"
#include "versioning.h"
#include "log.h"

/** 
 * Get file attributes.
 * 
 * ** FUSE **
 * Similar to stat().  The 'st_dev' and 'st_blksize' fields are
 * ignored.	 The 'st_ino' field is ignored except if the 'use_ino'
 * mount option is given.
 *
 * ** Hieronymus **
 * Currently this function is just a pass-through function. However,
 * it could be used to display versioning information next to the standard
 * information.
 */
int h_getattr (const char *path, struct stat *stat_buffer) 
{
    int return_value = 0;
    char root_path[PATH_MAX];

    resolve_root_path(path, root_path);

    return_value = lstat(root_path, stat_buffer);

    if (return_value != 0) {
        return_value = HIERONYMUS_ERROR(err_getattr, "h_getattr");
    }

    HIERONYMUS_DEBUG("getattr: %s\n", path);
    HIERONYMUS_LOG(ADMIN->log_file, "[%d] getattr # %s\n", OP_PID, path);

    return return_value;
}

/**
 * Read the target of a symbolic link
 * 
 * ** FUSE **
 * The buffer should be filled with a null terminated string.  The
 * buffer size argument includes the space for the terminating
 * null character.	If the linkname is too long to fit in the
 * buffer, it should be truncated.	The return value should be 0
 * for success.
 *
 * ** Hieronymus **
 * Pass through function.
 */
int h_readlink (const char *path, char *link, size_t size)
{
    int return_value = 0;
    char root_path[PATH_MAX];

    resolve_root_path(path, root_path);
    
    return_value = readlink(root_path, link, size);

    if (return_value < 0) {
        return_value = HIERONYMUS_ERROR(err_readlink, "h_readlink");
    }

    link[return_value] = '\0';

    HIERONYMUS_DEBUG("readlink: %s\n", path);
    HIERONYMUS_LOG(ADMIN->log_file, "[%d] readlink # %s\n", OP_PID, path);
    
    return return_value;
}

/**
 * Create a file node
 *
 * ** FUSE **
 * This is called for creation of all non-directory, non-symlink
 * nodes.  If the filesystem defines a create() method, then for
 * regular files that will be called instead.
 * 
 * ** Hieronymus **
 * Pass through function.
 *
 * NOTE: The file system defines a 'create()' method, so this function is never
 * called.
 */
int h_mknod (const char *path, mode_t mode, dev_t dev)
{
    int return_value = 0;
    char root_path[PATH_MAX];
    
    resolve_root_path(path, root_path);

    return_value = mknod(root_path, mode, dev);

    if (return_value < 0) {
        return_value = HIERONYMUS_ERROR(err_mknod, "h_mknod");
    }

    HIERONYMUS_DEBUG("mknod: %s\n", path);
    HIERONYMUS_LOG(ADMIN->log_file, "[%d] mknod # %s\n", OP_PID, path);
    
    return return_value;
}

/** 
 * Create a directory 
 *
 * ** FUSE **
 * Note that the mode argument may not have the type specification
 * bits set, i.e. S_ISDIR(mode) can be false.  To obtain the
 * correct directory type bits use  mode|S_IFDIR
 * 
 * ** Hieronymus **
 * When a directory is created a '.version' directory is directly created 
 * within the new directory.
 */
int h_mkdir (const char *path, mode_t mode)
{
    int return_value = 0;
    char root_path[PATH_MAX];

    resolve_root_path(path, root_path);
    
    /*
     * Make sure the mode is correct.
     */
    if (!S_ISDIR(mode)) {
        mode |= S_IFDIR;
    }

    return_value = checked_mkdir(root_path);

    if (return_value != 0) {
        return_value = HIERONYMUS_ERROR(err_mkdir, "h_mkdir");
    }

#ifdef _VERSIONING
    /*
     * Create the '.version' directory.
     */
    return_value = h_versioned_mkdir(root_path);
#endif

    HIERONYMUS_DEBUG("mkdir: %s\n", path);
    HIERONYMUS_LOG(ADMIN->log_file, "[%d] mkdir # %s\n", OP_PID, path);

    return return_value;
}

/** 
 * Remove a file 
 *
 * ** FUSE **
 * This function removes all links to a file, effectively deleting it.
 *
 * ** Hieronymus **
 * Removing a file has no direct effect for versioning. The last patch-version
 * can be used to retrieve the last version of the file.
 */
int h_unlink (const char *path)
{
    int return_value = 0;
    char root_path[PATH_MAX];
    
    resolve_root_path(path, root_path);

    return_value = unlink(root_path);

    if (return_value < 0) {
        return_value = HIERONYMUS_ERROR(err_unlink, "h_unlink");
    }
    
    HIERONYMUS_DEBUG("unlink: %s\n", path);
    HIERONYMUS_LOG(ADMIN->log_file, "[%d] unlink # %s\n", OP_PID, path);

    return return_value;
}

/**
 * Remove a directory 
 *
 * ** FUSE **
 *
 * ** Hieronymus **
 * Removing a directory amounts to moving it into the '.version'
 * directory of its parent directory. 
 */
int h_rmdir (const char *path)
{
    int return_value = 0;
    char root_path[PATH_MAX];

#ifdef _VERSIONING
    return_value = h_versioned_rmdir(path);
#else
    resolve_root_path(path, root_path);

    return_value = rmdir(root_path);
#endif

    if (return_value < 0) {
        return_value = HIERONYMUS_ERROR(err_rmdir, "h_rmdir");
    }
    
    HIERONYMUS_DEBUG("rmdir: %s\n", path);
    HIERONYMUS_LOG(ADMIN->log_file, "[%d] rmdir # %s\n", OP_PID, path);

    return return_value;
}

/**
 * Create a symbolic link 
 *
 * ** FUSE **
 *
 * ** Hieronymus **
 * Currently symbolic links are not versioned. Future updates might
 * add support for versioning symbolic links.
 */
int h_symlink (const char *path, const char *link)
{
    int return_value = 0;
    char root_link[PATH_MAX];
    
    resolve_root_path(link, root_link);
    
    return_value = symlink(path, root_link);

    if (return_value < 0) {
        return_value = HIERONYMUS_ERROR(err_symlink, "h_symlink");
    }
    
    HIERONYMUS_DEBUG("symlink: %s -> %s\n", path, link);
    HIERONYMUS_LOG(ADMIN->log_file, "[%d] symlink # %s -> %s\n", OP_PID, path, link);

    return return_value;
}

/** 
 * Rename a file 
 *
 * ** FUSE **
 *
 * ** Hieronymus **
 * Renaming a file has consequences for the consistency of the versioning
 * information. Currently versioning treats a move as the removal of the
 * original file and the creation of a new file.
 */
int h_rename (const char *path, const char *new_path)
{
    int return_value = 0;
    char root_path[PATH_MAX];
    char new_root_path[PATH_MAX];
    
    resolve_root_path(path, root_path);
    resolve_root_path(new_path, new_root_path);
    
    return_value = rename(root_path, new_root_path);

    if (return_value < 0) {
        return_value = HIERONYMUS_ERROR(err_rename, "h_rename");
    }
    
    HIERONYMUS_DEBUG("rename: %s ==> %s\n", path, new_path);
    HIERONYMUS_LOG(ADMIN->log_file, "[%d] rename # %s -> %s\n", OP_PID, path,
            new_path);

    return return_value;
}

/** 
 * Create a hard link to a file 
 * 
 * ** FUSE **
 *
 * ** Hieronymus **
 * Pass through function.
 */
int h_link (const char *path, const char *link_path)
{
    int return_value = 0;
    char root_path[PATH_MAX];
    char new_root_path[PATH_MAX];
    
    resolve_root_path(path, root_path);
    resolve_root_path(link_path, new_root_path);
    
    return_value = link(root_path, new_root_path);

    if (return_value < 0) {
        return_value = HIERONYMUS_ERROR(err_link, "h_link");
    }
    
    HIERONYMUS_DEBUG("link: %s -> %s\n", path, link_path);
    HIERONYMUS_LOG(ADMIN->log_file, "[%d] link # %s -> %s\n", OP_PID, path,
            link_path);

    return return_value;
}

/** 
 * Change the permission bits of a file 
 *
 * ** FUSE **
 *
 * ** Hieronymus **
 * Pass through function.
 */
int h_chmod (const char *path, mode_t mode)
{
    int return_value = 0;
    char root_path[PATH_MAX];

    resolve_root_path(path, root_path);

    return_value = chmod(root_path, mode);

    if (return_value < 0) {
        return_value = HIERONYMUS_ERROR(err_chmod, "h_chmod");
    }

    HIERONYMUS_DEBUG("chmod: %s\n", path);
    HIERONYMUS_LOG(ADMIN->log_file, "[%d] chmod # %s\n", OP_PID,  path);

    return return_value;
}

/** 
 * Change the owner and group of a file 
 *
 * ** FUSE **
 *
 * ** Hieronymus **
 * Pass through function.
 *
 * NOTE: It is quite unlikely that a user would change ownership 
 * of a file that is being monitored by Hieronymus.
 */
int h_chown (const char *path, uid_t uid, gid_t gid)
{
    int return_value = 0;
    char root_path[PATH_MAX];

    resolve_root_path(path, root_path);

    return_value = lchown(root_path, uid, gid);

    if (return_value < 0) {
        return_value = HIERONYMUS_ERROR(err_chown, "h_chown");
    }

    HIERONYMUS_DEBUG("chown: %s\n", path);
    HIERONYMUS_LOG(ADMIN->log_file, "[%d] chown # %s\n", OP_PID,  path);

    return return_value;
}

/**
 * Change the size of a file 
 *
 * ** FUSE **
 *
 * ** Hieronymus **
 * Truncating a file means a new version is stored and a patch from 
 * the previous version to this version is created.
 */
int h_truncate (const char *path, off_t new_size)
{
    int return_value = 0;
    char root_path[PATH_MAX];

    resolve_root_path(path, root_path);

    return_value = truncate(root_path, new_size);

    if (return_value < 0) {
        return_value = HIERONYMUS_ERROR(err_truncate, "h_truncate");
    }

    HIERONYMUS_DEBUG("truncate: %s\n", path);
    HIERONYMUS_LOG(ADMIN->log_file, "[%d] truncate # %s\n", OP_PID, path);

    return return_value;
}

/**
 * Change the access and/or modification times of a file 
 *
 * ** FUSE **
 *
 * ** Hieronymus **
 * Pass through function.
 */
int h_utime (const char *path, struct utimbuf *ubuffer)
{
    int return_value = 0;
    char root_path[PATH_MAX];
    
    resolve_root_path(path, root_path);
    
    return_value = utime(root_path, ubuffer);

    if (return_value < 0) {
        return_value = HIERONYMUS_ERROR(err_utime, "h_utime");
    }

    HIERONYMUS_DEBUG("utime: %s\n", path);
    HIERONYMUS_LOG(ADMIN->log_file, "[%d] utime # %s\n", OP_PID, path);
    
    return return_value;
}

/**
 * ** FUSE **
 * Change the access and modification times of a file with
 * nanosecond resolution.
 *
 * Introduced in version 2.6
 *
 * ** Hieronymus **
 * Pass through function.
 */
static int h_utimens (const char *path, const struct timespec ts[2])
{
    int return_value = 0;
	struct timeval tv[2];
    char root_path[PATH_MAX];
    
	tv[0].tv_sec = ts[0].tv_sec;
	tv[0].tv_usec = ts[0].tv_nsec / 1000;
	tv[1].tv_sec = ts[1].tv_sec;
	tv[1].tv_usec = ts[1].tv_nsec / 1000;

    resolve_root_path(path, root_path);

	return_value = utimes(root_path, tv);
	if (return_value == -1) {
        return_value = HIERONYMUS_ERROR(err_utimens, "h_utimens");
    }

    HIERONYMUS_DEBUG("utimens: %s\n", path);
    HIERONYMUS_LOG(ADMIN->log_file, "[%d] utimens # %s\n", OP_PID, path);

	return return_value;
}

/** 
 * File open operation
 *
 * ** FUSE **
 * No creation (O_CREAT, O_EXCL) and by default also no
 * truncation (O_TRUNC) flags will be passed to open(). If an
 * application specifies O_TRUNC, fuse first calls truncate()
 * and then open(). Only if 'atomic_o_trunc' has been
 * specified and kernel version is 2.6.24 or later, O_TRUNC is
 * passed on to open.
 *
 * Unless the 'default_permissions' mount option is given,
 * open should check if the operation is permitted for the
 * given flags. Optionally open may also return an arbitrary
 * filehandle in the fuse_file_info structure, which will be
 * passed to all file operations.
 *
 * Changed in version 2.2
 *
 * ** Hieronymus **
 * Upon opening a file Hieronymus calculate the SHA1 hash of its 
 * contents and stores it until the file is closed again.
 */
int h_open (const char *path, struct fuse_file_info *file_info)
{
    int return_value = 0;
    int file_descriptor = 0;
    char root_path[PATH_MAX];
    
    resolve_root_path(path, root_path);

    file_descriptor = open(root_path, file_info->flags);

    if (file_descriptor < 0) {
        return_value = HIERONYMUS_ERROR(err_open, "h_open");
    }
    
    file_info->fh = file_descriptor;
    
    HIERONYMUS_DEBUG("open: %s\n", path);
    HIERONYMUS_LOG(ADMIN->log_file, "[%d] open # %s\n", OP_PID, path);

    return return_value;
}

/**
 * Read data from an open file
 * 
 * ** FUSE **
 * Read should return exactly the number of bytes requested except
 * on EOF or error, otherwise the rest of the data will be
 * substituted with zeroes.	 An exception to this is when the
 * 'direct_io' mount option is specified, in which case the return
 * value of the read system call will reflect the return value of
 * this operation.
 *
 * Changed in version 2.2
 *
 * ** Hieronymus **
 * Pass through function.
 */
int h_read (const char *path, char *buffer, size_t size, off_t offset, 
        struct fuse_file_info *file_info)
{
    int return_value = 0;
    
    /*
     * We don't need to use the path here as the file handle is passed
     * directly through the fuse_file_info struct.
     */
    return_value = pread(file_info->fh, buffer, size, offset);

    if (return_value < 0) {
        return_value = HIERONYMUS_ERROR(err_read, "h_read");
    }
    
    HIERONYMUS_DEBUG("read: %s\n", path);
    HIERONYMUS_LOG(ADMIN->log_file, "[%d] read %d # %s\n", OP_PID,
            return_value, path);

    return return_value;
}

/** 
 * Write data to an open file
 *
 * ** FUSE **
 * Write should return exactly the number of bytes requested
 * except on error.	 An exception to this is when the 'direct_io'
 * mount option is specified (see read operation).
 *
 * Changed in version 2.2
 *
 * ** Hieronymus **
 * When a file is written, the file in the root directory is actually written.
 * If more than 0 bytes were written a new version is created. The latest
 * snapshot folder is opened and either a patch or snapshot version of the file
 * is created. A snapshot version is created if the snapshot folder does not yet
 * contain a reference to the file being written.
 */
int h_write (const char *path, const char *buffer, size_t size, off_t offset,
          struct fuse_file_info *file_info)
{
    int return_value = 0;
    
    return_value = pwrite(file_info->fh, buffer, size, offset);

#ifdef _VERSIONING
    char root_path[PATH_MAX];

    if (return_value > 0) {
        resolve_root_path(path, root_path);

        /*
         * We cannot overwrite return_value here as we would lose the amount of
         * bytes written to disk. That value is needed by FUSE to check if the
         * write succeeded.
         */
        if (h_versioned_write(root_path) < 0) {
            HIERONYMUS_ERROR(err_vs_write, "h_write");
        }
    }
#endif

    if (return_value < 0) {
        return_value = HIERONYMUS_ERROR(err_write, "h_write");
    }

    HIERONYMUS_DEBUG("write: %s\n %8s | buffer: %s\n", path, "", buffer);
    HIERONYMUS_LOG(ADMIN->log_file, "[%d] write %d bytes # %s\n", OP_PID,
            return_value, path);
    
    return return_value;
}

/** 
 * Get file system statistics
 *
 * ** FUSE **
 * The 'f_frsize', 'f_favail', 'f_fsid' and 'f_flag' fields are ignored
 *
 * Replaced 'struct statfs' parameter with 'struct statvfs' in
 * version 2.5
 *
 * ** Hieronymus **
 * Pass through function.
 */
int h_statfs (const char *path, struct statvfs *stat_info)
{
    int return_value = 0;
    char root_path[PATH_MAX];
    
    resolve_root_path(path, root_path);
    
    return_value = statvfs(root_path, stat_info);

    if (return_value < 0) {
        return_value = HIERONYMUS_ERROR(err_statfs, "h_statfs");
    }

    HIERONYMUS_DEBUG("statfs: %s\n", path);
    HIERONYMUS_LOG(ADMIN->log_file, "[%d] statfs # %s\n", OP_PID, path);
    
    return return_value;
}

/** 
 * Possibly flush cached data
 *
 * ** FUSE **
 * BIG NOTE: This is not equivalent to fsync().  It's not a
 * request to sync dirty data.
 *
 * Flush is called on each close() of a file descriptor.  So if a
 * filesystem wants to return write errors in close() and the file
 * has cached dirty data, this is a good place to write back data
 * and return any errors.  Since many applications ignore close()
 * errors this is not always useful.
 *
 * NOTE: The flush() method may be called more than once for each
 * open().	This happens if more than one file descriptor refers
 * to an opened file due to dup(), dup2() or fork() calls.	It is
 * not possible to determine if a flush is final, so each flush
 * should be treated equally.  Multiple write-flush sequences are
 * relatively rare, so this shouldn't be a problem.
 *
 * Filesystems shouldn't assume that flush will always be called
 * after some writes, or that if will be called at all.
 *
 * Changed in version 2.2
 *
 * ** Hieronymus **
 * Stub function.
 */
int h_flush (const char *path, struct fuse_file_info *file_info)
{
    (void) file_info;

    HIERONYMUS_DEBUG("flush: %s\n", path);
    HIERONYMUS_LOG(ADMIN->log_file, "[%d] flush # %s\n", OP_PID, path);

    return 0;
}

/** 
 * Release an open file
 *
 * ** FUSE **
 * Release is called when there are no more references to an open
 * file: all file descriptors are closed and all memory mappings
 * are unmapped.
 *
 * For every open() call there will be exactly one release() call
 * with the same flags and file descriptor.	 It is possible to
 * have a file opened more than once, in which case only the last
 * release will mean, that no more reads/writes will happen on the
 * file.  The return value of release is ignored.
 *
 * Changed in version 2.2
 *
 * ** Hieronymus **
 * Pass through function.
 */
int h_release (const char *path, struct fuse_file_info *file_info)
{
    int return_value = 0;

    return_value = close(file_info->fh);

    if (return_value < 0) {
        return_value = HIERONYMUS_ERROR(err_release, "h_release");
    }

    HIERONYMUS_DEBUG("release: %s\n", path);
    HIERONYMUS_LOG(ADMIN->log_file, "[%d] release # %s\n", OP_PID, path);

    return return_value;
}

/** 
 * Synchronize file contents
 *
 * ** FUSE **
 * If the data_sync parameter is non-zero, then only the user data
 * should be flushed, not the meta data.
 *
 * Changed in version 2.2
 *
 * ** Hieronymus **
 * Unused.
 *
 */
int h_fsync (const char *path, int data_sync, struct fuse_file_info *file_info)
{
    (void) data_sync;
    (void) file_info;

    HIERONYMUS_DEBUG("fsync: %s\n", path);
    HIERONYMUS_LOG(ADMIN->log_file, "[%d] fsync # %s\n", OP_PID, path);

    return 0;
}

/** 
 * Set extended attributes 
 *
 * ** FUSE **
 * 
 * ** Hieronymus **
 * Pass through function.
 */
int h_setxattr (const char *path, const char *name, const char *value, 
            size_t size, int flags)
{
    int return_value = 0;
    char root_path[PATH_MAX];
    
    resolve_root_path(path, root_path);
    
    return_value = lsetxattr(root_path, name, value, size, flags);

    if (return_value < 0) {
        return_value = HIERONYMUS_ERROR(err_setxattr, "h_setxattr");
    }

    HIERONYMUS_DEBUG("setxattr: %s (%s: %s)\n", path, name, value);
    HIERONYMUS_LOG(ADMIN->log_file, "[%d] setxattr # %s (%s: %s)\n", OP_PID,
            path, name, value);
    
    return return_value;
}

/**
 * Get extended attributes 
 *
 * ** FUSE **
 *
 * ** Hieronymus **
 * Pass through function.
 */
int h_getxattr (const char *path, const char *name, char *value, size_t size)
{
    int return_value = 0;
    char root_path[PATH_MAX];
    
    resolve_root_path(path, root_path);
    
    return_value = lgetxattr(root_path, name, value, size);

    if (return_value < 0) {
        return_value = HIERONYMUS_ERROR(err_getxattr, "h_getxattr");
    }
    
    HIERONYMUS_DEBUG("getxattr: %s (%s: %s)\n", path, name, value);
    HIERONYMUS_LOG(ADMIN->log_file, "[%d] getxattr # %s (%s: %s)\n", OP_PID,
            path, name, value);

    return return_value;
}

/** 
 * List extended attribute 
 *
 * ** FUSE **
 *
 * ** Hieronymus **
 * Pass through function.
 */
int h_listxattr (const char *path, char *list, size_t size)
{
    int return_value = 0;
    char root_path[PATH_MAX];
    char *ptr;
    
    resolve_root_path(path, root_path);
    
    return_value = llistxattr(root_path, list, size);

    if (return_value < 0) {
        return_value = HIERONYMUS_ERROR(err_listxattr, "h_listxattr");
    }
    
    HIERONYMUS_DEBUG("listxattr: %s:", path);
    HIERONYMUS_LOG(ADMIN->log_file, "[%d] listxattr # %s", OP_PID, path);

    for (ptr = list; ptr < list + return_value; ptr += strlen(ptr)+1) {
        HIERONYMUS_DEBUG("\t%s\n", ptr);
    }
    
    return return_value;
}

/** 
 * Remove extended attributes 
 *
 * ** FUSE **
 *
 * ** Hieronymus **
 * Pass through function.
 */
int h_removexattr (const char *path, const char *name)
{
    int return_value = 0;
    char root_path[PATH_MAX];
    
    resolve_root_path(path, root_path);
    
    return_value = lremovexattr(root_path, name);

    if (return_value < 0) {
        return_value = HIERONYMUS_ERROR(err_removexattr, "h_removexattr");
    }

    HIERONYMUS_DEBUG("removexattr: %s (%s)\n", path, name);
    HIERONYMUS_LOG(ADMIN->log_file, "[%d] removexattr # %s (%s)\n", OP_PID, path,
            name);
    
    return return_value;
}

/** 
 * Open directory
 *
 * ** FUSE **
 * Unless the 'default_permissions' mount option is given,
 * this method should check if opendir is permitted for this
 * directory. Optionally opendir may also return an arbitrary
 * filehandle in the fuse_file_info structure, which will be
 * passed to readdir, closedir and fsyncdir.
 *
 * Introduced in version 2.3
 *  
 * ** Hieronymus **
 * Pass through function.
 */
int h_opendir (const char *path, struct fuse_file_info *file_info)
{
    DIR *dir_pointer;
    int return_value = 0;
    char root_path[PATH_MAX];

    resolve_root_path(path, root_path);
    dir_pointer = opendir(root_path);

    if (dir_pointer == NULL) {
        return_value = HIERONYMUS_ERROR(err_opendir, "h_opendir");
    }

    file_info->fh = (intptr_t) dir_pointer;
    
    HIERONYMUS_DEBUG("opendir: %s\n", path);
    HIERONYMUS_LOG(ADMIN->log_file, "[%d] opendir # %s\n", OP_PID, path);

    return return_value; 
}

/** 
 * Read directory
 *
 * ** FUSE **
 * This supersedes the old getdir() interface.  New applications
 * should use this.
 *
 * The filesystem may choose between two modes of operation:
 *
 * 1) The readdir implementation ignores the offset parameter, and
 * passes zero to the filler function's offset.  The filler
 * function will not return '1' (unless an error happens), so the
 * whole directory is read in a single readdir operation.  This
 * works just like the old getdir() method.
 *
 * 2) The readdir implementation keeps track of the offsets of the
 * directory entries.  It uses the offset parameter and always
 * passes non-zero offset to the filler function.  When the buffer
 * is full (or an error happens) the filler function will return
 * '1'.
 *
 * Introduced in version 2.3
 *
 * ** Hieronymus **
 * To prevent the user from exploring the versioning information the '.version'
 * directory is removed from the directory listing.
 */
int h_readdir (const char *path, void *buffer, fuse_fill_dir_t filler, 
        off_t offset, struct fuse_file_info *file_info)
{
    int return_value = 0;
    DIR *dir_pointer;
    struct dirent *directory_entry;
    
    (void) offset;

    dir_pointer = (DIR *) (uintptr_t) file_info->fh;

    /*
     * As a directory always contains '.' and '..', the first call to readdir
     * should always return something, otherwise something is wrong.
     */
    directory_entry = readdir(dir_pointer);
    if (directory_entry == 0) {
        return_value = HIERONYMUS_ERROR(err_readdir, "h_readdir");
    }

    /*
     * Read all directory entries and feed them to the 'filler' callback.
     */
    do {
#ifdef _VERSIONING
        if (strncmp(directory_entry->d_name, ".version", 8) != 0) {
#endif
            /*
             * If filler returns a non-zero value it means the readdir buffer is
             * full, this is an error.
             */
            if (filler(buffer, directory_entry->d_name, NULL, 0) != 0) {
                return_value = HIERONYMUS_ERROR(err_rd_filler, "h_readdir");
            }
#ifdef _VERSIONING
        }
#endif
    } while ((directory_entry = readdir(dir_pointer)) != NULL);
    
    HIERONYMUS_DEBUG("readdir: %s\n", path);
    HIERONYMUS_LOG(ADMIN->log_file, "[%d] readdir # %s\n", OP_PID, path);

    return return_value;
}

/** 
 * Release directory
 *
 * ** FUSE **
 * Introduced in version 2.3
 *
 * ** Hieronymus **
 * Pass through function.
 */
int h_releasedir (const char *path, struct fuse_file_info *file_info)
{
    int return_value = 0;
    
    return_value = closedir((DIR *) (uintptr_t) file_info->fh);

    if (return_value < 0) {
        return_value = HIERONYMUS_ERROR(err_releasedir, "h_releasedir");
    }

    HIERONYMUS_DEBUG("releasedir: %s\n", path);
    HIERONYMUS_LOG(ADMIN->log_file, "[%d] releasedir # %s\n", OP_PID, path);
    
    return return_value;
}

/** 
 * Synchronize directory contents
 *
 * ** FUSE **
 * If the data_sync parameter is non-zero, then only the user data
 * should be flushed, not the meta data
 *
 * Introduced in version 2.3
 * 
 * ** Hieronymus **
 * Stub function.
 */
int h_fsyncdir (const char *path, int data_sync, struct fuse_file_info *file_info)
{
    (void) data_sync;
    (void) file_info;

    HIERONYMUS_DEBUG("fsyncdir: %s\n", path);
    HIERONYMUS_LOG(ADMIN->log_file, "[%d] fsyncdir # %s\n", OP_PID, path);

    return 0;
}

/**
 * Initialize filesystem
 *
 * ** FUSE **
 * The return value will passed in the private_data field of
 * fuse_context to all file operations and as a parameter to the
 * destroy() method.
 *
 * Introduced in version 2.3
 * Changed in version 2.6
 *
 * ** Hieronymus **
 * Unimplemented.
 *
 */
void *h_init (struct fuse_conn_info *connection)
{
    (void) connection;

    HIERONYMUS_NOTE("init\n");

    return ADMIN;
}

/**
 * Clean up filesystem
 *
 * ** FUSE **
 *
 * Called on filesystem exit.
 *
 * Introduced in version 2.3
 *
 * ** Hieronymus **
 * Free up the memory used by the private-data field.
 */
void h_destroy (void *user_data)
{
    if (user_data != NULL) {
        free(user_data);
    }

    HIERONYMUS_NOTE("destroy\n");
}

/**
 * Check file access permissions
 *
 * ** FUSE **
 * This will be called for the access() system call.  If the
 * 'default_permissions' mount option is given, this method is not
 * called.
 *
 * This method is not called under Linux kernel versions 2.4.x
 *
 * Introduced in version 2.5
 *
 * ** Hieronymus **
 * Pass through function.
 */
int h_access (const char *path, int mask)
{
    int return_value = 0;
    char root_path[PATH_MAX];

    resolve_root_path(path, root_path);
   
    return_value = access(root_path, mask);
    
    if (return_value < 0) {
        return_value = HIERONYMUS_ERROR(err_access, "h_access");
    }

    HIERONYMUS_DEBUG("access: %s\n", path);
    HIERONYMUS_LOG(ADMIN->log_file, "[%d] access # %s\n", OP_PID, path);
    
    return return_value;
}

/**
 * Create and open a file
 *
 * ** FUSE **
 * If the file does not exist, first create it with the specified
 * mode, and then open it.
 *
 * If this method is not implemented or under Linux kernel
 * versions earlier than 2.6.15, the mknod() and open() methods
 * will be called instead.
 *
 * Introduced in version 2.5
 *
 * ** Hieronymus **
 * Pass through function.
 */
int h_create (const char *path, mode_t mode, struct fuse_file_info *file_info)
{
    int return_value = 0;
    char root_path[PATH_MAX];
    int file_descriptor;
    
    resolve_root_path(path, root_path);
    
    file_descriptor = creat(root_path, mode);

    if (file_descriptor < 0) {
        return_value = HIERONYMUS_ERROR(err_create, "h_create");
    }
    
    file_info->fh = file_descriptor;
    
    HIERONYMUS_DEBUG("create: %s\n", path);
    HIERONYMUS_LOG(ADMIN->log_file, "[%d] create # %s\n", OP_PID, path);

    return return_value;
}

/**
 * Change the size of an open file
 *
 * ** FUSE **
 * This method is called instead of the truncate() method if the
 * truncation was invoked from an ftruncate() system call.
 *
 * If this method is not implemented or under Linux kernel
 * versions earlier than 2.6.15, the truncate() method will be
 * called instead.
 *
 * Introduced in version 2.5
 *
 * ** Hieronymus **
 * Pass through function.
 */
int h_ftruncate (const char *path, off_t offset, 
        struct fuse_file_info *file_info)
{
    int return_value = 0;
    
    return_value = ftruncate(file_info->fh, offset);

    if (return_value < 0) {
        return_value = HIERONYMUS_ERROR(err_ftruncate, "h_ftruncate");
    }
    
    HIERONYMUS_DEBUG("ftruncate: %s\n", path);
    HIERONYMUS_LOG(ADMIN->log_file, "[%d] ftruncate # %s\n", OP_PID, path);

    return return_value;
}

/**
 * Get attributes from an open file
 *
 * ** FUSE **
 * This method is called instead of the getattr() method if the
 * file information is available.
 *
 * Currently this is only called after the create() method if that
 * is implemented (see above).  Later it may be called for
 * invocations of fstat() too.
 *
 * Introduced in version 2.5
 *
 * ** Hieronymus **
 * Could be a place to add versioning information. Currently just a pass through
 * function.
 */
int h_fgetattr (const char *path, struct stat *stat_buffer, 
        struct fuse_file_info *file_info)
{
    int return_value = 0;
    
    return_value = fstat(file_info->fh, stat_buffer);

    if (return_value < 0) {
        return_value = HIERONYMUS_ERROR(err_fgetattr, "h_fgetattr");
    }

    HIERONYMUS_DEBUG("fgetattr: %s\n", path);
    HIERONYMUS_LOG(ADMIN->log_file, "[%d] fgetattr # %s\n", OP_PID, path);
    
    return return_value;
}

/**
 * Acquire the path to the actual contents of the file system.
 *
 * ** Hieronymus **
 * The root of the file system and the mount point are two separate locations in
 * the file system. However, all paths are relative to the mount point. This
 * function translates each path to the root directory.
 */
void resolve_root_path (const char *path, char *new_path) 
{
    HIERONYMUS_DEBUG("resolve_root_path: %s\n", path);
    
    sprintf((char *) new_path, "%s%s", ADMIN->root_directory, path);
    new_path[strlen(ADMIN->root_directory) + strlen(path)] = '\0';
}

/**
 * Struct containing the addresses of all the FUSE callback functions.
 */
struct fuse_operations hieronymus_operations = {
    .getattr = h_getattr,
    .readlink = h_readlink,
    .mknod = h_mknod,
    .mkdir = h_mkdir,
    .unlink = h_unlink,
    .rmdir = h_rmdir,
    .symlink = h_symlink,
    .rename = h_rename,
    .link = h_link,
    .chmod = h_chmod,
    .chown = h_chown,
    .truncate = h_truncate,
    .utime = h_utime,
    .utimens = h_utimens,
    .open = h_open,
    .read = h_read,
    .write = h_write,
    .statfs = h_statfs,
    .flush = h_flush,
    .release = h_release,
    .fsync = h_fsync,
    .setxattr = h_setxattr,
    .getxattr = h_getxattr,
    .listxattr = h_listxattr,
    .removexattr = h_removexattr,
    .opendir = h_opendir,
    .readdir = h_readdir,
    .releasedir = h_releasedir,
    .fsyncdir = h_fsyncdir,
    .init = h_init,
    .destroy = h_destroy,
    .access = h_access,
    .create = h_create,
    .ftruncate = h_ftruncate,
    .fgetattr = h_fgetattr
};

/**
 * Main
 *
 * ** Hieronymus **
 * This is where all preparation takes place, i.e.:
 *
 *  - Parsing the commandline.
 *  - Adding optional commandline arguments.
 *  - Creating the necessary directories.
 *  - Syncing root directory and mount directory (Unimplemented).
 *  - Starting the FUSE main loop.
 */
int main (int argc, char *argv[])
{
    int fuse_stat = 0;
    int i = 1;
    char versioning_root[PATH_MAX] = "";
    hieronymus_data *administration = NULL;

    /* Handle custom commandline parameters */
    parse_commandline(argc, argv, versioning_root);


    /*
     * Check if a custom versioning root was set (and consequently removed from
     * the commandline arguments).
     */
    if (strlen(versioning_root) > 0) {

#ifdef _DEBUG
        printf("NOTE: Custom versioning root directory: %s\n", versioning_root);
#endif
        argc--;
    }
    
    /*
     * This argument ensures Fuse accepts that the mountpoint is non-empty at
     * mount-time.
     */
    argc = add_commandline_arg(argc, &argv, "-o nonempty");

#ifdef _DEBUG
    argc = add_commandline_arg(argc, &argv, "-f");
 //   argc = add_commandline_arg(argc, &argv, "-d");
#endif
    
    for(; i < argc; i++) {
        if (strncmp(argv[i], "-", 1) != 0) {
            break;
        }
    }

    /*
     * Create ~/.hieronymus (or custom root) if it doesn't exist. 
     */
    if (create_versioning_root(versioning_root, argv[i]) < 0) {
        HIERONYMUS_ERROR(err_versioning_root, "main");
        abort();
    }

    /* 
     * Synchronize mountpoint and versioning_root. 
     *
     * TODO
     * This function would be needed when mounting an existing (non-empty)
     * directory with versioning.
     */
    //synchronize_roots();

    /*
     * Setup private data-structure. 
     */
    administration = calloc(sizeof(hieronymus_data), 1);
    if (administration == NULL) {
        HIERONYMUS_ERROR(err_malloc, "main");
        abort();
    }

    administration->root_directory = versioning_root;
    administration->max_num_versions = MAX_NUM_VERSIONS;
    administration->log_file = open_log_file();

    umask(0);

    /*
     * Start fuse with some extra options (such as nonempty). 
     */
    fuse_stat = fuse_main(argc, argv, &hieronymus_operations, administration);
    
    /* 
     * Synchronize versioning_root and mountpoint (reverse direction from before
     * the FUSE loop). 
     *
     * TODO
     */
    //synchronize_roots();

    return fuse_stat;
}
