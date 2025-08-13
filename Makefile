TARGET = river
SRC = river_raid.c

ifeq ($(OS),Windows_NT)
	CC = gcc
	CFLAGS =
	LDFLAGS =
else
	CC = gcc
	CFLAGS =
	LDFLAGS = -lncurses
endif

all:
	$(CC) -o $(TARGET) $(SRC) $(CFLAGS) $(LDFLAGS)

clean:
	rm -f $(TARGET)
