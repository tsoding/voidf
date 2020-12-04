CFLAGS=-Wall -Wextra -std=c11 -pedantic `pkg-config --cflags x11`
LIBS=`pkg-config --libs x11`

voidf: main.c
	$(CC) $(CFLAGS) -o voidf main.c $(LIBS)
