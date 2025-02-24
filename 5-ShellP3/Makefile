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
	rm -f dragon.o

# Clean up build files
clean:
	rm -f $(TARGET)

test:
	bats $(wildcard ./bats/*.sh)

valgrind:
	echo "pwd\nexit" | valgrind --leak-check=full --show-leak-kinds=all --error-exitcode=1 ./$(TARGET) 
	echo "pwd\nexit" | valgrind --tool=helgrind --error-exitcode=1 ./$(TARGET) 

# Phony targets
.PHONY: all clean test
