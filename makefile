CC = gcc
INCLUDE = -I/usr/include -I/usr/local/include
LIBS = -lm -ldl -ludev -pthread -lxml2

SHARED_FLAGS = -DCOLOUR_QUANTISATION -DLIMITED_FREQ -DPLANE_SPAN_TEXTURING
DEBUG_FLAGS =  -g -DDEBUG -DDEBUG_VALUES #-DDEBUG_BORDERS #-DSUPPRESS_FRAMEBUFFER_OUTPUT
RELEASE_FLAGS =  -O3 -ffast-math  -march=native

SOURCES = main.c src/graphics.c src/terminal.c src/physics.c src/io.c src/ui.c src/loader.c
OBJECTS = $(SOURCES:.c=.o)
BINFILE = prgm.x86_64



.PHONY: all release debug clean

all: release

release: CFLAGS = $(SHARED_FLAGS) $(RELEASE_FLAGS)
release: $(OBJECTS)
	$(CC) $(OBJECTS) $(LIBS) -o $(BINFILE)

debug: CFLAGS = $(SHARED_FLAGS) $(DEBUG_FLAGS)
debug: $(OBJECTS)
	$(CC) $(OBJECTS) $(LIBS) -o $(BINFILE)

%.o: %.c %.h
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(BINFILE)


