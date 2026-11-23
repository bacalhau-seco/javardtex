RAYLIB ?= /usr/local/include/

all:
	gcc src/*.c -I $(RAYLIB) -L $(RAYLIB) -std=c99 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 -o game
