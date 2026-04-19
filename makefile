CC = gcc
CFLAGS = 
INCLUDE = -I/usr/include -I/usr/local/include

LIBS = -lm -ldl -pthread


SOURCES = main.c src/terminal.c src/io.c
OBJECTS = $(SOURCES:.c=.o)
BINFILE = prgm.x86_64



.PHONY: all release debug clean

all: release

release: CFLAGS := -O2 -ffast-math
release: $(OBJECTS)
	$(CC) $(OBJECTS) $(LIBS) -o $(BINFILE)

debug: CFLAGS := -g -DDEBUG -DCOLOUR_QUANTISATION
debug: $(OBJECTS)
	$(CC) $(OBJECTS) $(LIBS) -o $(BINFILE)

%.o: %.c %.h
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(BINFILE)


