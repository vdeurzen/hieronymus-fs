/******************************************************************************
 *
 * file   : versioning.h
 *
 * author : Tim van Deurzen
 * date   : 09/01/2011
 *
 * Functions implementing the versioned variants of the function relevant for
 * versioning.
 *
 *****************************************************************************/

#include <fuse.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "versioning.h"
#include "util.h"
#include "error.h"
#include "fuse_main.h"

/**
 * Create a new directory and its '.version' directory.
 *
 * When a new directory is created a directory called '.version' is created
 * within the new directory. This '.version' directory will contain all
 * versioning data.
 */
int h_versioned_mkdir(const char *root_path) 
{
    char tmp[PATH_MAX];
    int return_value;

    /*
     * We're creating a new directory so using checked_mkdir here would 
     * add some unnecessary overhead.
     */
    sprintf(tmp, "%s/.version", root_path);

    return_value = mkdir(tmp, S_IRWXU | S_IRWXG);

    if (return_value != 0) {
        return_value = HIERONYMUS_ERROR(err_mkdir, "h_versioned_mkdir");
    }

    return return_value;
}

/**
 * Remove a directory.
 *
 * When a directory is removed, it is in fact simply moved into its
 * parent-directory's '.version' folder. That way it can be restored if
 * necessary.
 */
int h_versioned_rmdir(const char *path)
{
    int return_value = 0;

    char tmp1[PATH_MAX];
    char tmp2[PATH_MAX];
    parent_directory(path, tmp1);
    bottom_directory(path, tmp2);

    sprintf(tmp1, "%s/.version/__DIR__%s__%s", tmp1, tmp2, timestamp());

    return_value = rename(path, tmp1);

    if (return_value != 0) {
        return_value = HIERONYMUS_ERROR(err_rename, "h_versioned_rmdir");
    }

    return return_value;
}

/**
 * Write a file to disk.
 *
 * This function implements the most important step for versioning in
 * Hieronymus. This function is only called if more than 0 bytes are written. In
 * that case it figures out if we're dealing with a snapshot version or a patch
 * version. For a snapshot version it copies the file to the latest snapshot
 * directory. For a patch version it creates a patch in the latest snapshot
 * directory.
 */
int h_versioned_write(const char *path)
{
    int return_value = 0;
    int num_versions = 0;
    char snapshot_path[PATH_MAX];
    char directory[PATH_MAX];
    char filename[MAX_FILENAME];

    parent_directory(path, directory);
    bottom_directory(path, filename);
    strncat(directory, "/.version", 9);
    
    /* Find latest snapshot (with this file), if it exists. 
     *
     * Find-function creates the first snapshot folder if necessary else it
     * returns the newest snapshot folder (possibly without this file).
     */
    if (find_latest_snapshot(directory, snapshot_path) < 0) {
        return_value = HIERONYMUS_ERROR(err_snapshot, "h_versioned_write");
    }

    sprintf(snapshot_path, "%s/%s", snapshot_path, filename);
    strncpy(directory, snapshot_path, (strlen(snapshot_path) - strlen(filename)));
    directory[(strlen(snapshot_path) - strlen(filename)) - 1] = '\0';

    /* 
     * Check if this file exists in the snapshot folder, if not, simply make a
     * copy of the file (i.e. snapshot version). Else call diff with the
     * snapshot version and the new version and store the patch.
     */
    if ((num_versions = find_snapshot_version(directory, filename)) < 0) {
        return_value = copy(path, snapshot_path);
    } else {
        /*
         * If we've exceeded the maximum number of versions per snapshot, create
         * a new snapshot.
         */
        if (num_versions > ADMIN->max_num_versions) {
            parent_directory(path, directory);
            strncat(directory, "/.version", 9);
            make_snapshot_directory(directory, snapshot_path);

            HIERONYMUS_DEBUG("num_versions: %d, snapshot_dir: %s", 
                    num_versions, snapshot_path);
        } 

        return_value = diff(snapshot_path, path);
    }

    return return_value;
}
