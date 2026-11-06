RAYLIB ?= /usr/local/include/

all:
	gcc src/main.c -I $(RAYLIB) -L $(RAYLIB) -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 -o game
