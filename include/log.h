/******************************************************************************
 *
 * file   : log.h
 *
 * author : Tim van Deurzen
 * date   : 27/01/2011
 *
 * Contains all functionality needed for logging purposes.
 *
 *****************************************************************************/

#ifdef _LOGGING
#define HIERONYMUS_LOG(fh, format, ...) \
    fprintf(fh, format, __VA_ARGS__)
#else
#define HIERONYMUS_LOG(...)
#endif


FILE *open_log_file (void);
