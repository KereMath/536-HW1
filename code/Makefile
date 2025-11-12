# Makefile for CEng 536 HW1

CC = gcc
CFLAGS = -Wall -Wextra -g -pthread -std=gnu11 -D_GNU_SOURCE
LDFLAGS = -lpthread -lrt

TARGET = hw1
SRCS = hw1.c heap.c
OBJS = $(SRCS:.c=.o)

# Main target
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

# Compile .c to .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean
clean:
	rm -f $(TARGET) $(OBJS)

# Debug build
debug: CFLAGS += -DDEBUG -O0
debug: clean $(TARGET)

# Test with Unix domain socket
test-unix: $(TARGET)
	./$(TARGET) @/tmp/gym.sock 1000 5000 3

# Test with TCP socket
test-tcp: $(TARGET)
	./$(TARGET) 127.0.0.1:5678 1000 5000 3

.PHONY: clean debug test-unix test-tcp