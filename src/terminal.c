/* terminal.c */
#include <string.h>
#include <fxcg/heap.h>

#include <fxcg/display.h>

#include "types.h"
#include "maths.h"



Buffer_t framebuffer;





//////// FRAMEBUFFER ////////
//////// INITIALISATION ////////
void t_createFramebuffer(void) {
	//Overwrites the current instance of framebuffer (if valid) with a new FB.
	framebuffer.data = (RGB_t*)(GetVRAMAddress()); //Get VRAM ptr
}

RGB_t* t_getFramebufferPTR(void) {return framebuffer.data;}

void t_deleteFramebuffer(void) {
	if (!framebuffer.valid) {return; /* Already deleted. */}
	sys_free(framebuffer.data);
	framebuffer.valid = FALSE;
}
//////// INITIALISATION ////////





//////// DIRECT DRAW ////////
void t_writePX(const Vec2i_t position, RGB_t colour) {
	if (!framebuffer.valid) {return;}
	if (
		(position.x < 0.0f) || (position.y < 0.0f) ||
		(position.x >= LCD_WIDTH_PX) ||
		(position.y >= LCD_HEIGHT_PX)
	) {
		return; //Out of the FB.
	}

	rgb_quantise(&colour);

	framebuffer.data[(int)((position.y * LCD_WIDTH_PX) + position.x)] = colour;
}


RGB_t t_readPX(const Vec2i_t position) {
	if (!framebuffer.valid) {return (RGB_t){0u};}
	if (
		(position.x < 0.0f) || (position.y < 0.0f) ||
		(position.x >= LCD_WIDTH_PX) ||
		(position.y >= LCD_HEIGHT_PX)
	) {
		return (RGB_t){0u}; //Out of the FB.
	}

	return framebuffer.data[(int)((position.y * LCD_WIDTH_PX) + position.x)];
}
//////// DIRECT DRAW ////////



void t_drawFramebuffer(void) {
	Bdisp_PutDisp_DD(); //Push VRAM to screen
}



void t_clearFramebuffer(void) {
	Bdisp_AllClr_VRAM(); //Clear VRAM.
}

void t_fillFramebuffer(RGB_t colour) {
	//Fill with single colour.
	Bdisp_AllClr_VRAM(); //Clear VRAM.
	Bdisp_Fill_VRAM(colour, 3);
}
//////// FRAMEBUFFER ////////
