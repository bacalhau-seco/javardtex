CC = clang
TARGET = build/javardtex

LUA_CFLAGS = $(shell pkg-config --cflags lua5.4)
LUA_LIBS   = $(shell pkg-config --libs lua5.4)

CFLAGS  = -std=c99 -Werror -Iinclude -Wall -Wextra $(LUA_CFLAGS)
CFLAGS2 = -std=c99 -Iinclude -Wall -Wextra $(LUA_CFLAGS)

SRC = src/*.c

LIB = -Llib -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 $(LUA_LIBS)

RAYLIB_DIR = external/raylib/src

HEADERS = \
	$(RAYLIB_DIR)/raylib.h \
	$(RAYLIB_DIR)/raymath.h \
	$(RAYLIB_DIR)/rlgl.h \
	$(RAYLIB_DIR)/rcamera.h \
	$(RAYLIB_DIR)/rgestures.h

.PHONY: all run clean force raylib

all: raylib
	$(CC) $(CFLAGS) $(SRC) $(LIB) -o $(TARGET)

run: raylib
	$(CC) $(CFLAGS) $(SRC) $(LIB) -o $(TARGET)
	./$(TARGET)

force: raylib
	$(CC) $(CFLAGS2) $(SRC) $(LIB) -o $(TARGET)
	./$(TARGET)

raylib:
	$(MAKE) -C $(RAYLIB_DIR)
	mkdir -p include lib
	cp $(HEADERS) include/
	cp $(RAYLIB_DIR)/libraylib.a lib/

clean:
	$(MAKE) -C $(RAYLIB_DIR) clean
	rm -f $(TARGET)
	rm -f lib/libraylib.a
	rm -f $(patsubst $(RAYLIB_DIR)/%,include/%,$(HEADERS))
