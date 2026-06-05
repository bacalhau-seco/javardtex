CC = clang
TARGET = build/idek

CFLAGS= -std=c99 -Iinclude -Werror -Wall -Wextra
SRC = src/*
LIB =  -Llib -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

all: 
	$(CC) $(CFLAGS) $(SRC) $(LIB) -o $(TARGET) 

run:
	$(CC) $(CFLAGS) $(SRC) $(LIB) -o $(TARGET) && ./build/idek

clean:
	rm -f $(TARGET)
