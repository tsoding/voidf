PKGS=x11 sdl2
CFLAGS=-Wall -Wextra -std=c11 -pedantic `pkg-config --cflags $(PKGS)`
LIBS=`pkg-config --libs $(PKGS)`

voidf: main.c
	$(CC) $(CFLAGS) -o voidf main.c $(LIBS)
