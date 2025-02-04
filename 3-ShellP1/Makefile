# Compiler settings
CC = gcc
CFLAGS = -Wall -Wextra -g

# Target executable name
TARGET = dsh

# Find all source and header files
SRCS = $(wildcard *.c)
HDRS = $(wildcard *.h)

DRAGON_FILE = dragon.bin

# Default target
all: $(TARGET)

create_dragon: $(DRAGON_FILE)
	ld -r -b binary -o dragon.o $(DRAGON_FILE)

# Compile source to executable
$(TARGET): $(SRCS) $(HDRS) create_dragon
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS) dragon.o

# Clean up build files
clean:
	rm -f $(TARGET)
	rm -rf dsh.dSYM
	rm -f dragon.o

test: $(TARGET)
	./test.sh

# Phony targets
.PHONY: all clean
