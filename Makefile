CC=gcc
CFLAGS=-Iheader
LDFLAGS=-lSDL3 -lSDL3_image
TARGET=out/game
SRC=$(wildcard src/*.c)

$(TARGET):$(SRC)
	$(CC) $(SRC) $(LDFLAGS) $(CFLAGS) -o $(TARGET)

	./$(TARGET)
