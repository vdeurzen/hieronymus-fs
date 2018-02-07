/******************************************************************************
 *
 * file   : versioning.h
 *
 * author : Tim van Deurzen
 * date   : 09/01/2011
 *
 *****************************************************************************/

#ifndef _HIERONYMUS_VERSIONING_H
#define _HIERONYMUS_VERSIONING_H

#define MAX_FILENAME 256


int h_versioned_mkdir(const char *);

int h_versioned_rmdir(const char *);

int h_versioned_create(const char *, mode_t);

int h_versioned_unlink(const char *);

#endif
