/******************************************************************************
 *
 * file   : util.c
 *
 * author : Tim van Deurzen
 * date   : 06/01/2011
 *
 * Contains utility function used by the Hieronymus Versioning File System.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <fuse.h>
#include <sys/types.h>
#include <limits.h>
#include <pwd.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>

#include "util.h"
#include "fuse_main.h"
#include "error.h"
#include "sha1.h"
#include "print_color.h"

#define MAX_TIME_STR 64

/**
 * Create the versioning root directory and a mountpoint-specific subdirectory.
 *
 * The mountpoint-specific subdirectory is indicated by the SHA1 hash of the
 * path of the actual mountpoint.
 */
int create_versioning_root(char *versioning_root, char *mount_directory) 
{
    unsigned int uid = 0;
    struct passwd *user_info = NULL;
    char mount_id[MAX_ID_LENGTH] = "";
    char tmp[PATH_MAX] = "",
         cwd[PATH_MAX] = "";

    /*
     * If no custom versioning root is set use a hidden folder in the
     * home-directory of the user running Hieronymus.
     */
    if (strlen(versioning_root) == 0) {
        if ((uid = getuid()) == -1) {
            HIERONYMUS_ERROR(err_getuid, "create_versioning_root");
            return -1;
        }

        if ((user_info = getpwuid(uid)) == NULL) {
            HIERONYMUS_ERROR(err_getpwuid, "create_versioning_root");
            return -1;
        }

        sprintf(versioning_root, "%s/%s", user_info->pw_dir, ".hieronymus");
    }


    /*
     * Remove the final slash in the path, so we always have a consistent
     * mountpoint 'path-format'.
     */
    if (mount_directory[strlen(mount_directory) - 1] == '/') {
        mount_directory[strlen(mount_directory) - 1] = '\0';
    }

    (void) getcwd(cwd, sizeof(cwd));
    sprintf(tmp, "%s/%s", cwd, mount_directory);

    /*
     * Calculate SHA1 of the mount-path. 
     *
     * The SHA1 of the mount-path represents the mountpoint inside the
     * versioning root directory.
     *
     *    ``/path/to/versioning/root/<sha1_of_mountpoint>/...''
     */
    sha1_str(tmp, mount_id);
    
    /* 
     * Make the root directory unless it already exists.
     */
    if (checked_mkdir(versioning_root) < 0) {
        return -1;
    }
    
    sprintf(tmp, "%s/%s", versioning_root, mount_id);
    strncpy(versioning_root, tmp, strlen(tmp));
    versioning_root[strlen(tmp)] = '\0';
    
    /* 
     * Make the mountpoint-specific versioning root directory,
     * unless it already exists.
     */
    if (checked_mkdir(versioning_root) < 0) {
        return -1;
    }

#ifdef _VERSIONING
    /*
     * Make the '.version' directory in the versioning root.
     */
    sprintf(tmp, "%s/.version", versioning_root);

    return checked_mkdir(tmp);
#else
    return 0;
#endif

}

/**
 * Create a new snapshot directory inside the '.version' directory.
 *
 * This function expects an absolute path to the '.version' directory (inside
 * the root-directory, not under the mount-point).
 */
int make_snapshot_directory(const char *path, char *new_path)
{
    sprintf(new_path, "%s/%s", path, timestamp());

    return checked_mkdir(new_path);
}

/**
 * Search the '.version' directory for the latest snapshot directory.
 *
 * This function finds the latest snapshot directory by comparing the timestamps
 * of each snapshot. If there is no snapshot yet, this function will create the
 * first. The function copies the path to the snapshot directory to its second
 * arugment.
 */
int find_latest_snapshot(const char *path, char *new_path)
{
    int return_value = 0;
    DIR *dir_pointer;
    struct dirent *directory_entry;

    int latest = 0;
    int current = 0;
    
    dir_pointer = opendir(path);

    if (dir_pointer == NULL) {
        return HIERONYMUS_ERROR(err_opendir, "find_latest_snapshot");
    }

    /*
     * As a directory always contains '.' and '..', the first call to readdir
     * should always return something, otherwise something is wrong.
     */
    directory_entry = readdir(dir_pointer);
    if (directory_entry == 0) {
        return_value = HIERONYMUS_ERROR(err_readdir, "find_latest_snapshot");
    }

    do {
        /*
         * Next to '.' and '..' the '.version' directory should only contain
         * snapshot folders.
         */
        if (strncmp(directory_entry->d_name, ".", 1) != 0) {
            current = atoi(directory_entry->d_name);

            HIERONYMUS_DEBUG("current: %d, latest: %d\n", current, latest);
            if (current > latest) {
                latest = current;
            }
        }
    } while ((directory_entry = readdir(dir_pointer)) != NULL);

    if (latest == 0) {
        return_value = make_snapshot_directory(path, new_path);
    } else {
        sprintf(new_path, "%s/%d", path, latest);
    }

    return return_value;
}

/**
 * Determine if a file has a snapshot version inside the latest snapshot
 * directory.
 *
 * This function returns the number of versions in this snapshot, counting
 * starts at -1 to indicate the absence of a snapshot version. So, 0 means a
 * snapshot version is available, and anything above 0 is the number of versions
 * in this snapshot.
 */
int find_snapshot_version (const char *path, const char *filename) 
{
    DIR *dir_pointer;
    struct dirent *directory_entry;
    int filename_length = strlen(filename);
    int number_of_versions = -1;

    dir_pointer = opendir(path);

    if (dir_pointer == NULL) {
        return HIERONYMUS_ERROR(err_opendir, "find_snapshot_version");
    }

    /*
     * As a directory always contains '.' and '..', the first call to readdir
     * should always return something, otherwise something is wrong.
     */
    directory_entry = readdir(dir_pointer);
    if (directory_entry == 0) {
        return HIERONYMUS_ERROR(err_readdir, "find_latest_snapshot");
    }

    do {
        /*
         * Determine the number of versions of the file.
         */
        if (strncmp(directory_entry->d_name, filename, filename_length) == 0) {
            number_of_versions++;
        }
    } while ((directory_entry = readdir(dir_pointer)) != NULL);

    return number_of_versions;
}

/**
 * Create a directory.
 *
 * Check if a directory exists and if so exit successfully and if
 * not, create the directory.
 */
int checked_mkdir(const char *path)
{
    int return_value = 0;
    DIR *dir_pointer;

    dir_pointer = opendir(path);

    if (dir_pointer == NULL) {
        return_value = mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH); 

        if (return_value != 0) {
            HIERONYMUS_ERROR(err_mkdir, "checked_mkdir");
            return -1;
        }
    }

    free(dir_pointer);

    return 0;
}


/**
 * Calculate the SHA1 hash of a string.
 *
 * Returns a string representation of the HASH instead of an 'untranslated'
 * characted array.
 */
void sha1_str(const char *string, char *output) 
{
    int i = 0;
    unsigned char sha1_output[SHA1_LENGTH];
    char tmp[2];

    sha1((const unsigned char *)string, strlen(string), sha1_output);

    /*
     * Construct the string representation of the SHA1 hash.
     */
    for(; i < SHA1_LENGTH; i++ ) {
        sprintf(tmp, "%02x", sha1_output[i] );
        strcat(output, tmp);
    }
}

/**
 * Allocate memory
 *
 * An intermediate function for allocating memory. This function allocates the
 * memory and verifies the operation was successful. If allocation was
 * unsuccessful execution is aborted.
 */
void *checked_malloc(int size)
{
    void *pointer = NULL;

    pointer = malloc(size);

    if (pointer == NULL) {
        HIERONYMUS_ERROR(err_malloc, "checked_malloc");
        abort();
    }

    return pointer;
}

/**
 * Function to start outputting debug information (possibly colorized).
 */
inline void start_print_debug(void) 
{
    START_PRINT_BLUE();

    fprintf(stderr, "*** DEBUG | ");
}

/**
 * Function to stop outputting debug information (possibly colorized).
 */
inline void end_print_debug(void) 
{
    END_PRINT_COLOR();
}

/**
 * Get the parent directory.
 *
 * Delete the 'bottom-most' directory from the path, giving the parent
 * directory. The path to the parent directory is put in dest.
 */
void parent_directory(const char *path, char *dest) 
{
    int i = strlen(path);

    while(i > 0 && path[i--] != '/');

    strncpy(dest, path, i + 1);
    dest[i + 2] = '\0';
}

/**
 * Get the bottom-most directory.
 *
 * Extract the last part of the path, givin the bottom-most directory. 
 * The path to / name of the bottom-most directory is put in dest.
 */
void bottom_directory(const char *path, char *dest)
{
    int i = strlen(path);

    while(i > 0 && path[i--] != '/');

    strncpy(dest, path + (i + 2), strlen(path) - i);
    dest[(strlen(path) - i) + 1] = '\0';
}

/**
 * Return a formatted string containing the date and time.
 */
char *timestamp(void)
{
    char *timestamp;

    timestamp = (char *) checked_malloc(64 * sizeof(char));

    struct tm *time_struct;
    time_t time_value;

    time_value = time(NULL);
    time_struct = localtime(&time_value);

    strftime(timestamp, MAX_TIME_STR, "%s", time_struct);

    return timestamp;
}

/**
 * Copy a file from source to dest.
 *
 * This function uses 'system' to call the function 'cp'. If anything goes wrong
 * it is reported as an error.
 */
int copy (const char *source, const char *dest)
{
    int return_value = 0;
    char command[MAX_COMMAND_LENGTH];

    sprintf(command, "cp %s %s", source, dest);

    return_value = system(command);

    if (return_value < 0) {
        return_value = HIERONYMUS_ERROR(err_system, "copy");
    }

    HIERONYMUS_NOTE("copy: creating snapshot version.\n");

    return return_value;
}

/**
 * Calculate the diff between old_file and new_file.
 *
 * This function creates a patch file to go from old_file to new_file.
 * The patch can be generated by xdelta or gnudiff, which one is used can be
 * changed at compile-time (-D_XDELTA for xdelta).
 */
int diff (const char *old_file, const char *new_file)
{
    int return_value = 0;
    char command[MAX_COMMAND_LENGTH];

#ifdef _XDELTA
    sprintf(command, "xdelta3 -e -s %s %s %s-%s.patch", 
            old_file, new_file, old_file, timestamp());
#else
    sprintf(command, "diff -u %s %s > %s-%s.patch", 
            old_file, new_file, old_file, timestamp());
#endif

    return_value = system(command);

    if (return_value < 0) {
        return_value = HIERONYMUS_ERROR(err_system, "diff");
    }

    HIERONYMUS_NOTE("diff: creating patch version.\n");

    return return_value;
}
