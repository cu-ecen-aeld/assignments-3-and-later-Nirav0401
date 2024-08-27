# Is compiled by the “all” and “default” target of a Makefile in the “server” directory and supports cross compilation, placing the executable file in the “server” directory and named aesdsocket.

# Variables
CC ?= gcc
CFLAGS ?= -g -Wall -Werror
LDFLAGS ?= -lpthread -lrt

# Check if CROSS_COMPILE is defined
ifdef CROSS_COMPILE
CC = $(CROSS_COMPILE)gcc
endif

# Source files
SRC = aesdsocket.c timer.c
OBJ = $(SRC:.c=.o)

# Default target: build the aesdsocket application
all: aesdsocket

# Rule to build the aesdsocket application
aesdsocket: $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

# Rule to build object files
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Clean target to remove compiled files
clean:
	rm -f aesdsocket $(OBJ)

.PHONY: all clean

