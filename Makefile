CC = clang
TARGET = build/javardtex

LUA_CFLAGS = $(shell pkg-config --cflags lua5.4)
LUA_LIBS = $(shell pkg-config --libs lua5.4)

CFLAGS = -std=c99 -Werror -Iinclude -Wall -Wextra -O3 $(LUA_CFLAGS)

SRC = src/*.c

LIB = -Llib -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 $(LUA_LIBS)

RAYLIB_DIR = external/raylib/src

# Options forwarded as-is to raylib's own Makefile, e.g.
#   make RAYLIB_FLAGS="RAYLIB_BUILD_MODE=DEBUG GRAPHICS=GRAPHICS_API_OPENGL_21"
# Changing them needs a rebuild of raylib: make raylib-clean raylib
RAYLIB_FLAGS ?=

HEADERS = \
	$(RAYLIB_DIR)/raylib.h \
	$(RAYLIB_DIR)/raymath.h \
	$(RAYLIB_DIR)/rlgl.h \
	$(RAYLIB_DIR)/rcamera.h \
	$(RAYLIB_DIR)/rgestures.h

.PHONY: all run clean force raylib raylib-clean

all: raylib
	cp -r game/ build/
	$(CC) $(CFLAGS) $(SRC) $(LIB) -o $(TARGET)

run: raylib
	$(CC) $(CFLAGS) $(SRC) $(LIB) -o $(TARGET)
	./$(TARGET)

raylib:
	$(MAKE) -C $(RAYLIB_DIR) $(RAYLIB_FLAGS)
	mkdir -p include lib
	cp $(HEADERS) include/
	cp $(RAYLIB_DIR)/libraylib.a lib/

raylib-clean:
	$(MAKE) -C $(RAYLIB_DIR) $(RAYLIB_FLAGS) clean

clean: raylib-clean
	rm -f $(TARGET)
	rm -f lib/libraylib.a
	rm -f $(patsubst $(RAYLIB_DIR)/%,include/%,$(HEADERS))
