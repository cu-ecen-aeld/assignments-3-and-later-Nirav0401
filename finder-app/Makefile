# Variables
CC = gcc
CFLAGS = -Wall -Werror
LDFLAGS =

# Check if CROSS_COMPILE is defined
ifdef CROSS_COMPILE
CC = $(CROSS_COMPILE)gcc
endif

# Source files
SRC = writer.c
OBJ = $(SRC:.c=.o)

# Default target: build the writer application
all: writer

# Rule to build the writer application
writer: $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

# Rule to build object files
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Clean target to remove compiled files
clean:
	rm -f writer $(OBJ)

.PHONY: all clean

