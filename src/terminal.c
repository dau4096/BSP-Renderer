/* terminal.c */


#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>


#include "types.h"
#include "maths.h"
#include "ui.h"



Buffer_t framebuffer;




//////// UTILITY ////////
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





//////// STRING UTILITY ////////
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



void t_clearLowestNLines(unsigned int N) {
	if (framebuffer.resolutionCHARS.y < N) {return;}
	unsigned int startRow = framebuffer.resolutionCHARS.y - N + 1u;
	printf("\033[%u;1H", startRow); //Move to first of bottom N lines
    printf("\033[J"); //Clear then onward.
}


void t_resetCursor(void) {
	t_clearLowestNLines(UI_HEIGHT);
	printf("\x1b[1;1H"); //Moves cursor to the top-left.
}
//////// STRING UTILITY ////////





#define PREFIX_SIZE 5u /* <pre> */
#define SUFFIX_SIZE 6u /* </pre> */
#define SPAN_START_SIZE 61u /* <span style="background-color:#XXXXXX"><font color="#YYYYYY"> */
#define SPAN_END_SIZE 14u /* </font></span> */
#define NEWLINE_SIZE 1u /* \n | 0x0A */

unsigned int t_getHTMLbufSize(void) {
	unsigned int bufSize = PREFIX_SIZE + SPAN_START_SIZE + SPAN_END_SIZE + SUFFIX_SIZE;
	RGB_t topPrev = RGB_BLACK;RGB_t lowPrev = RGB_BLACK;
	int hasReset = TRUE; ///May not be accurate, force a colour change.

	for (uint y=framebuffer.resolutionPX.y-2; y>0u; y-=2u) {
		for (uint x=0u; x<framebuffer.resolutionPX.x; x++) {

			int topIndex = (y * framebuffer.resolutionPX.x) + x;
			int lowIndex = ((y + 1) * framebuffer.resolutionPX.x) + x;

			RGB_t top = (y + 1 < framebuffer.resolutionPX.y) ? framebuffer.backData[lowIndex] : RGB_BLACK;
			RGB_t low = framebuffer.backData[topIndex];


			//Check if the colour needs to change.
			if (hasReset || (top.r != topPrev.r) || (top.g != topPrev.g) || (top.b != topPrev.b)) {
				bufSize += SPAN_END_SIZE + SPAN_START_SIZE; //Start new span.
				topPrev = top;
			}
			if (hasReset || (low.r != lowPrev.r) || (low.g != lowPrev.g) || (low.b != lowPrev.b)) {
				bufSize += SPAN_END_SIZE + SPAN_START_SIZE; //Start new span.
				lowPrev = low;
			}


			bufSize++; //Represents adding a UTF8 "▀" char
			hasReset = FALSE; //Has not reset, free to assume continuous colour.
		}

		bufSize += NEWLINE_SIZE; //Adding a newline.
		topPrev = RGB_BLACK;
		lowPrev = RGB_BLACK;
		hasReset = TRUE;
	}


	return bufSize;
}



char* t_getColourCode(const RGB_t colour) {
	char* buf = calloc(8, sizeof(char));
	sprintf(
		buf, "#%02X%02X%02X",
		colour.r, colour.g, colour.b
	);
	return buf;
}


void t_getSpanStartChars(char** buf, const RGB_t bg, const RGB_t fg) {
	sprintf(
		*buf, "<span style=\"background-color:%s\"><font color=\"%s\">",
		t_getColourCode(bg), t_getColourCode(fg)
	);
}


void t_getHTML(char** buf, unsigned int* size) {
	*size = 0u;
	if (!framebuffer.valid) {return;}

	*size = t_getHTMLbufSize();
	*buf = (char*)(calloc(*size, sizeof(char)));
	char* bufPTR = *buf;


	char* PREFIX_CHARS = "<pre>";
	char* SUFFIX_CHARS = "</pre>";
	char* SPAN_START_CHARS = calloc(SPAN_START_SIZE, sizeof(char));
	char* SPAN_END_CHARS = "</font></span>";
	char* NEWLINE_CHARS = "\n";

	strAppend(bufPTR, PREFIX_CHARS);
	strAppend(bufPTR, SPAN_START_CHARS);


	RGB_t topPrev = RGB_BLACK;
	RGB_t lowPrev = RGB_BLACK;
	int hasReset = TRUE; ///May not be accurate, force a colour change.

	for (uint y=framebuffer.resolutionPX.y-2; y>0u; y-=2u) {
		for (uint x=0u; x<framebuffer.resolutionPX.x; x++) {

			int topIndex = (y * framebuffer.resolutionPX.x) + x;
			int lowIndex = ((y + 1) * framebuffer.resolutionPX.x) + x;

			RGB_t top = (y + 1 < framebuffer.resolutionPX.y) ? framebuffer.backData[lowIndex] : RGB_BLACK;
			RGB_t low = framebuffer.backData[topIndex];


			//Check if the colour needs to change.
			if (hasReset || (top.r != topPrev.r) || (top.g != topPrev.g) || (top.b != topPrev.b)) {
				strAppend(bufPTR, SPAN_END_CHARS); bufPTR += SPAN_END_SIZE; //End previous span.
				t_getSpanStartChars(&SPAN_START_CHARS, low, top);
				strAppend(bufPTR, SPAN_START_CHARS); bufPTR += SPAN_START_SIZE; //Start new span.
				topPrev = top;
			}
			if (hasReset || (low.r != lowPrev.r) || (low.g != lowPrev.g) || (low.b != lowPrev.b)) {
				strAppend(bufPTR, SPAN_END_CHARS); bufPTR += SPAN_END_SIZE; //End previous span.
				t_getSpanStartChars(&SPAN_START_CHARS, low, top);
				strAppend(bufPTR, SPAN_START_CHARS); bufPTR += SPAN_START_SIZE; //Start new span.
				lowPrev = low;
			}


			*bufPTR++ = '\xE2'; //UTF8 "▀" char
			*bufPTR++ = '\x96';
			*bufPTR++ = '\x80';
			hasReset = FALSE; //Has not reset, free to assume continuous colour.
		}

		strAppend(bufPTR, NEWLINE_CHARS); bufPTR += NEWLINE_SIZE; //Adding a newline.
		topPrev = RGB_BLACK;
		lowPrev = RGB_BLACK;
		hasReset = TRUE;
	}

	strAppend(bufPTR, SPAN_END_CHARS);
	strAppend(bufPTR, SUFFIX_CHARS);

	free(SPAN_START_CHARS);
}
//////// UTILITY ////////








//////// FRAMEBUFFER ////////
//////// INITIALISATION ////////
void t_createFramebuffer(const Vec2i_t resolutionChars) {
	//Overwrites the current instance of framebuffer (if valid) with a new FB.
	
	Vec2i_t resolution = (Vec2i_t){resolutionChars.x, resolutionChars.y*2};
	if ((resolution.x <= 0.0f) || (resolution.y <= 0.0f)) {return; /* Invalid resize */}

	if (framebuffer.valid) {
		//Currently has an active framebuffer, free old memory.
		free(framebuffer.frontData);
		free(framebuffer.backData);
	}

	framebuffer.resolutionPX = resolution;
	framebuffer.resolutionCHARS = resolutionChars;
	framebuffer.frontData = calloc((int)(resolution.x * resolution.y), sizeof(RGB_t)); //allocate.
	framebuffer.backData = calloc((int)(resolution.x * resolution.y), sizeof(RGB_t)); //allocate.
	framebuffer.valid = (framebuffer.frontData != NULL) && (framebuffer.backData != NULL);
}

RGB_t* t_getFramebufferPTR(void) {return framebuffer.frontData;}

void t_deleteFramebuffer(void) {
	if (!framebuffer.valid) {return; /* Already deleted. */}
	free(framebuffer.frontData);
	free(framebuffer.backData);
	framebuffer.resolutionPX = (Vec2i_t){0, 0};
	framebuffer.resolutionCHARS = (Vec2i_t){0, 0};
	framebuffer.valid = FALSE;
}
//////// INITIALISATION ////////





//////// DIRECT DRAW ////////
void t_writePX(const Vec2i_t position, RGB_t colour) {
	if (!framebuffer.valid) {return;}
	if (
		(position.x < 0.0f) || (position.y < 0.0f) ||
		(position.x >= framebuffer.resolutionPX.x) ||
		(position.y >= framebuffer.resolutionPX.y)
	) {
		return; //Out of the FB.
	}

	rgb_quantise(&colour);

	framebuffer.frontData[(int)((position.y * framebuffer.resolutionPX.x) + position.x)] = colour;
}


RGB_t t_readPX(const Vec2i_t position) {
	if (!framebuffer.valid) {return (RGB_t){0u, 0u, 0u};}
	if (
		(position.x < 0.0f) || (position.y < 0.0f) ||
		(position.x >= framebuffer.resolutionPX.x) ||
		(position.y >= framebuffer.resolutionPX.y)
	) {
		return (RGB_t){0u, 0u, 0u}; //Out of the FB.
	}

	return framebuffer.frontData[(int)((position.y * framebuffer.resolutionPX.x) + position.x)];
}
//////// DIRECT DRAW ////////





#define WIDTH  (framebuffer.resolutionPX.x)
#define HEIGHT (framebuffer.resolutionPX.y)
void t_drawFramebuffer(void) {
	if (!framebuffer.valid) {return; /* Invalid, Can't show. */}

	size_t bufferSize = (size_t)(framebuffer.resolutionPX.x * (framebuffer.resolutionPX.y/2 + 1) * 64);
	char *buffer = malloc(bufferSize); //Start
	char *out = buffer; //End

	out = strAppend(out, "\x1b[0m"); //Definitely reset.
	RGB_t topPrev = RGB_BLACK;
	RGB_t lowPrev = RGB_BLACK;
	int hasReset = TRUE; ///May not be accurate, force a colour change.

	for (uint y=framebuffer.resolutionPX.y-2; y>0u; y-=2u) {
		for (uint x=0u; x<framebuffer.resolutionPX.x; x++) {

			int topIndex = (y * framebuffer.resolutionPX.x) + x;
			int lowIndex = ((y + 1) * framebuffer.resolutionPX.x) + x;

			RGB_t top = (y + 1 < framebuffer.resolutionPX.y) ? framebuffer.backData[lowIndex] : RGB_BLACK;
			RGB_t low = framebuffer.backData[topIndex];


			//Check if the colour needs to change.
			if (hasReset || (top.r != topPrev.r) || (top.g != topPrev.g) || (top.b != topPrev.b)) {
				out = setForeground(out, top);
				topPrev = top;
			}
			if (hasReset || (low.r != lowPrev.r) || (low.g != lowPrev.g) || (low.b != lowPrev.b)) {
				out = setBackground(out, low);
				lowPrev = low;
			}


			*out++ = '\xE2'; //UTF8 "▀" char
			*out++ = '\x96';
			*out++ = '\x80';
			hasReset = FALSE; //Has not reset, free to assume continuous colour.
		}

		out = strAppend(out, "\x1b[0m\n"); //Reset formatting.
		topPrev = RGB_BLACK;
		lowPrev = RGB_BLACK;
		hasReset = TRUE;
	}

	fwrite(buffer, 1, out - buffer, stdout);
	free(buffer);
}


void t_clearFramebuffer(void) {
	//Fills with black, slightly quicker than the loop method.
	memset(framebuffer.frontData, 0x00u, (framebuffer.resolutionPX.x * framebuffer.resolutionPX.y) * sizeof(RGB_t));
}


void t_fillFramebuffer(RGB_t colour) {
	//Fill with single colour.
	//Compiler (hopefully) optimises this nicer!
	RGB_t* end = framebuffer.frontData + (framebuffer.resolutionPX.x * framebuffer.resolutionPX.y);
	for (RGB_t* ptr=framebuffer.frontData; ptr<end; ptr++) {
		*ptr = colour;
	}
}


void t_swapBuffers(void) {
	RGB_t* temporary = framebuffer.backData;
	framebuffer.backData = framebuffer.frontData;
	framebuffer.frontData = temporary;
}
//////// FRAMEBUFFER ////////
