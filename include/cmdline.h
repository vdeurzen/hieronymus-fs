/******************************************************************************
 *
 * file   : cmdline.h 
 *
 * author : Tim van Deurzen
 * date   : 06/01/2011
 * 
 * Prototypes and macros for processing the commandline arguments.
 *
 *****************************************************************************/

#ifndef __HIERONYMUS_CMDLINE_H
#define __HIERONYMUS_CMDLINE_H

#define MAX_ARG_LENGTH 128


void parse_commandline(int, char **, char *);

int add_commandline_arg(int, char ***, char *);

#endif
