/* terminal.c */


#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>


#include "types.h"



Buffer_t framebuffer;



#ifndef _WIN32
//Linux only method.
#include <sys/ioctl.h>
#include <unistd.h>
Vec2i_t t_getTerminalSize() {
	//Gets size of the terminal window, in characters.
	struct winsize w;

	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

	return (Vec2i_t){
		.x = w.ws_col,
		.y = w.ws_row,
	};
}

#else
//Windows only method.
#include <windows.h>

Vec2i_t t_getTerminalSize(void) {
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);

	return (Vec2i_t){
		.x = csbi.srWindow.Right - csbi.srWindow.Left + 1,
		.y = csbi.srWindow.Bottom - csbi.srWindow.Top - 1
	};
}

#endif


void t_createFramebuffer(const Vec2i_t resolution) {
	//Overwrites the current instance of framebuffer (if valid) with a new FB.
	if ((resolution.x <= 0.0f) || (resolution.y <= 0.0f)) {return; /* Invalid resize */}

	if (framebuffer.valid) {
		//Currently has an active framebuffer, free old memory.
		free(framebuffer.data);
	}

	framebuffer.resolution = resolution;
	framebuffer.data = calloc((int)(resolution.x * resolution.y), sizeof(RGB_t)); //allocate.
	framebuffer.valid = (framebuffer.data != NULL);
}

RGB_t* t_getFramebufferPTR(void) {return framebuffer.data;}

void t_deleteFramebuffer(void) {
	if (!framebuffer.valid) {return; /* Already deleted. */}
	free(framebuffer.data);
	framebuffer.resolution = (Vec2i_t){0, 0};
	framebuffer.valid = FALSE;
}


void t_quantise(RGB_t* colour) {
#ifdef COLOUR_QUANTISATION //Only modifies if necessary.
	colour->r = (colour->r >> 4) << 4;
	colour->g = (colour->g >> 4) << 4;
	colour->b = (colour->b >> 4) << 4;
#endif
}


void t_writePX(const Vec2i_t position, RGB_t colour) {
	if (!framebuffer.valid) {return;}
	if (
		(position.x < 0.0f) || (position.y < 0.0f) ||
		(position.x >= framebuffer.resolution.x) ||
		(position.y >= framebuffer.resolution.y)
	) {
		return; //Out of the FB.
	}

	t_quantise(&colour);

	framebuffer.data[(int)((position.y * framebuffer.resolution.x) + position.x)] = colour;
}


RGB_t t_readPX(const Vec2i_t position) {
	if (!framebuffer.valid) {return (RGB_t){0u, 0u, 0u};}
	if (
		(position.x < 0.0f) || (position.y < 0.0f) ||
		(position.x >= framebuffer.resolution.x) ||
		(position.y >= framebuffer.resolution.y)
	) {
		return (RGB_t){0u, 0u, 0u}; //Out of the FB.
	}

	return framebuffer.data[(int)((position.y * framebuffer.resolution.x) + position.x)];
}




static inline char* strAppend(char *dst, const char *src) {
    while (*src) {*dst++ = *src++;} //Append using ptrs.
    return dst;
}


static inline char* intAppend(char *dst, int v) {
	//Append an integer, formatted correctly for an ANSI escape code.
    char tmp[12];
    int i = 0;

    if (v == 0) {
        *dst++ = '0';
        return dst;
    }

    while (v > 0) {
        tmp[i++] = '0' + (v % 10);
        v /= 10;
    }

    while (i--) {*dst++ = tmp[i];}
    return dst;
}


static inline char* setForeground(char *out, const RGB_t c) {
	*out++ = '\x1b'; *out++ = '['; *out++ = '3'; *out++ = '8'; *out++ = ';'; *out++ = '2'; *out++ = ';'; //"\x1b[38;2;"
    out = intAppend(out, c.r); *out++ = ';';
    out = intAppend(out, c.g); *out++ = ';';
    out = intAppend(out, c.b); *out++ = 'm';
    return out;
}

static inline char* setBackground(char *out, const RGB_t c) {
	*out++ = '\x1b'; *out++ = '['; *out++ = '4'; *out++ = '8'; *out++ = ';'; *out++ = '2'; *out++ = ';'; //"\x1b[48;2;"
    out = intAppend(out, c.r); *out++ = ';';
    out = intAppend(out, c.g); *out++ = ';';
    out = intAppend(out, c.b); *out++ = 'm';
    return out;
}



void t_resetCursor(void) {
	printf("\x1b[1;1H"); //Moves cursor to the top-left.
}


#define WIDTH  (framebuffer.resolution.x)
#define HEIGHT (framebuffer.resolution.y)
void t_drawFramebuffer(void) {
    if (!framebuffer.valid) {return; /* Invalid, Can't show. */}

    size_t bufferSize = (size_t)(WIDTH * (HEIGHT/2 + 1) * 64);
    char *buffer = malloc(bufferSize); //Start
    char *out = buffer; //End

    RGB_t topPrev = RGB_WHITE;
	RGB_t lowPrev = RGB_BLACK;

    for (uint y=0u; y<HEIGHT; y+=2u) {
        for (uint x=0u; x<WIDTH; x++) {

            int topIndex = (y * WIDTH) + x;
            int lowIndex = ((y + 1) * WIDTH) + x;

            RGB_t top = framebuffer.data[topIndex];
            RGB_t low = (y + 1 < HEIGHT) ? framebuffer.data[lowIndex] : RGB_BLACK;


            //Check if the colour needs to change.
            if ((top.r != topPrev.r) || (top.g != topPrev.g) || (top.b != topPrev.b)) {
	            out = setForeground(out, top);
			    topPrev = top;
			}
			if ((low.r != lowPrev.r) || (low.g != lowPrev.g) || (low.b != lowPrev.b)) {
	            out = setBackground(out, low);
			    lowPrev = low;
			}


            *out++ = '\xE2'; //UTF8 "▀" char
            *out++ = '\x96';
            *out++ = '\x80';
        }

        out = strAppend(out, "\x1b[0m\n"); //Reset formatting.
    	topPrev = RGB_WHITE;
		lowPrev = RGB_BLACK;
    }

    fwrite(buffer, 1, out - buffer, stdout);
    free(buffer);
}


void t_clearFramebuffer(void) {
	//Fills with black, slightly quicker than the loop method.
	memset(framebuffer.data, 0x00u, (framebuffer.resolution.x * framebuffer.resolution.y) * sizeof(RGB_t));
}

void t_fillFramebuffer(RGB_t colour) {
	//Fill with single colour.
	//Compiler (hopefully) optimises this nicer!
	RGB_t* end = framebuffer.data + (framebuffer.resolution.x * framebuffer.resolution.y);
	for (RGB_t* ptr=framebuffer.data; ptr<end; ptr++) {
		*ptr = colour;
	}
}