CC = gcc

CFLAGS = -Wall -Wextra -Wpedantic -Iinclude -D_XOPEN_SOURCE=700

SRC = src/main.c \
      src/server.c \
      src/http.c \
      src/mime.c \
      src/files.c

TARGET = minihttpd

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all clean