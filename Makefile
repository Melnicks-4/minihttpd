CC      = gcc
CFLAGS  = -Wall -Wextra -Werror -std=gnu11 -g -Iinclude
TARGET  = minihttpd
SRCS    = $(wildcard src/*.c)
OBJS    = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

run: all
	./$(TARGET) 8080

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all run clean
