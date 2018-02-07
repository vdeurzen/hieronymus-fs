#
# Makefile for building Hieronymus FS.
#

CFLAGS  = -Wall -ggdb -D_FILE_OFFSET_BITS=64 -I/usr/local/include/fuse
CFLAGS += -D_LOGGING #-D_DEBUG -D_PRINT_COLOR -D_VERSIONING -D_XDELTA -D_SUPPRESS_ERRORS
LDFLAGS = -lfuse -lpthread -lrt -ldl

.PHONY: all clean

MAIN = hieronymus

# C compiler information.
CC = gcc

# Pre-compiler flags.
CPPFLAGS = -I include

# Output macro
OUTPUT = -o $@

# Compilation macro.
COMPILE = $(CC) $(CPPFLAGS) $(CFLAGS) -c

# Linking macro.
LINK = $(CC) $(CPPFLAGS) $(CFLAGS) $(OUTPUT) $^ $(LDFLAGS)

# Specific path for source, documentation and header files.
VPATH = src include

# Build any necessary object files.
%.o: %.c
	@echo "[Compiling] $<"
	@$(COMPILE) $< $(OUTPUT)

all: $(MAIN)

hieronymus: fuse_main.o cmdline.o util.o error.o sha1.o versioning.o log.o
	@echo "[Linking] $@"
	@$(LINK)

clean:
	@echo "[Cleaning temporary files]"
	@rm -f *.o
