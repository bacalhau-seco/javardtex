CC = clang
TARGET = build/javardtex

CFLAGS= -std=c99 -Werror -Iinclude -Wall -Wextra
CFLAGS2= -std=c99 -Iinclude -Wall -Wextra
SRC = src/*.c
LIB = -Llib -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

all: 
	$(CC) $(CFLAGS) $(SRC) $(LIB) -o $(TARGET) 

run:
	$(CC) $(CFLAGS) $(SRC) $(LIB) -o $(TARGET) && ./build/idek

clean:
	rm -f $(TARGET)

force:
	$(CC) $(CFLAGS2) $(SRC) $(LIB) -o $(TARGET) && ./build/idek
