RAYLIB ?= /usr/local/include/

all:
	gcc src/main.c -I $(RAYLIB) -L $(RAYLIB) -Wall -Wextra -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 -o game
