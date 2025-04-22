CC=gcc
CFLAGS=-lSDL3
TARGET=game.exe
SRC=$(wildcard *.c)

$(TARGET):$(SRC)
	$(CC) $(SRC) $(CFLAGS) -o $(TARGET)

	./$(TARGET)