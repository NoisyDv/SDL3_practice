CC=gcc
CFLAGS=-Iheader -Wall -Wextra -g
LDFLAGS=-lSDL3 -lSDL3_image -lm
TARGET=out/game
SRC=$(wildcard src/*.c)

$(TARGET):$(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

.PHONY: run
