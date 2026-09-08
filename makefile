CC = /usr/local/cross/bin/sh3eb-elf-gcc
CFLAGS = -I$(HOME)/libfxcg/include -I./ -I./src -O2 -Wall -Wno-unused-function -fno-lto -m4-nofpu -std=c99
LIBS = -L$(HOME)/libfxcg/lib -lfxcg -lc -lgcc -nostdlib -nostartfiles
LKR = $(HOME)/libfxcg/toolchain/prizm.x


SOURCES = main.c src/graphics.c src/terminal.c src/physics.c src/io.c src/ui.c src/loader.c
OBJECTS = $(SOURCES:.c=.o)
BINFILE = BSP.bin
G3AFILE = $(BINFILE:.bin=.g3a)
NAME = "BSP Renderer"



.PHONY: all bin g3a clean

all: g3a

bin: $(OBJECTS)
	$(CC) $(OBJECTS) $(LIBS) -T $(LKR) -o "$(BINFILE)"

g3a: bin
	mkg3a -n $(NAME) -i uns:ico/unselected.bmp -i sel:ico/selected.bmp "$(BINFILE)" "$(G3AFILE)"

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) "$(BINFILE)" *.g3a


