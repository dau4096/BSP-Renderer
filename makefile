CC = gcc
S = ./src
E = ./editor

INCLUDE = -I/usr/include -I/usr/local/include
PRGM_LIBS = -lm -ldl -ludev -pthread -lxml2
EDITOR_LIBS = -lm -ludev -lxml2


SHARED_FLAGS = -DCOLOUR_QUANTISATION -DLIMITED_FREQ -DPLANE_SPAN_TEXTURING
DEBUG_FLAGS = -g -O0 -DDEBUG -DDEBUG_VALUES -DDEBUG_DRAW_ORDER #-DDEBUG_BORDERS #-DSUPPRESS_FRAMEBUFFER_OUTPUT
RELEASE_FLAGS = -O3 -ffast-math -march=native
EDITOR_FLAGS = -g -O0 -DEDITOR #-DSUPPRESS_FRAMEBUFFER_OUTPUT


PRGM_SOURCES = ./main.c $(S)/graphics.c $(S)/terminal.c $(S)/physics.c $(S)/io.c $(S)/ui.c $(S)/file.c $(S)/multithread.c
PRGM_OBJECTS = $(PRGM_SOURCES:.c=.o)
EDITOR_SOURCES = $(E)/main.c $(E)/terminal.c $(S)/io.c $(S)/file.c $(E)/graphics.c $(E)/edit.c $(E)/menu.c
EDITOR_OBJECTS = $(EDITOR_SOURCES:.c=.o)

PRGM_BINFILE = ./prgm.x86_64
EDITOR_BINFILE = ./editor.x86_64


.PHONY: all release debug clean

all: release

release: CFLAGS = $(SHARED_FLAGS) $(RELEASE_FLAGS)
release: $(PRGM_OBJECTS)
	$(CC) $(PRGM_OBJECTS) $(PRGM_LIBS) -o $(PRGM_BINFILE)

debug: CFLAGS = $(SHARED_FLAGS) $(DEBUG_FLAGS)
debug: $(PRGM_OBJECTS)
	$(CC) $(PRGM_OBJECTS) $(PRGM_LIBS) -o $(PRGM_BINFILE)

editor: CFLAGS = $(EDITOR_FLAGS)
editor: $(EDITOR_OBJECTS)
	$(CC) $(EDITOR_OBJECTS) $(EDITOR_LIBS) -o $(EDITOR_BINFILE)

%.o: %.c %.h
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

clean:
	rm -f $(PRGM_OBJECTS) $(PRGM_BINFILE)

editor-clean:
	rm -f $(EDITOR_OBJECTS) $(EDITOR_BINFILE)


