RAYLIB ?= /usr/local/include/

all:
	gcc src/*.c -I $(RAYLIB) -L $(RAYLIB) -Wall -Wextra -lGL -lm -lpthread -ldl -lrt -lX11 -o game
