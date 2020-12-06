PKGS=sdl2
CFLAGS_COMMON=-Wall -Wextra -std=c11 -pedantic
CFLAGS=$(CFLAGS_COMMON) `pkg-config --cflags $(PKGS)`
LIBS=`pkg-config --libs $(PKGS)` -lm

voidf: main.c image.h
	$(CC) $(CFLAGS) -o voidf main.c $(LIBS)

image.h: image2c
	./image2c charmap-oldschool.bmp > image.h

image2c: image2c.c stb_image.h
	$(CC) $(CFLAGS_COMMON) -o image2c image2c.c -lm
