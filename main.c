/* main.c */

#include <stdio.h>

#include "src/types.h"
#include "src/terminal.h"


//Will be used later to add a UI bar at the bottom.
#define UI_HEIGHT 0u


int main(void) {

	Vec2i_t tResChars = t_getTerminalSize();
	tResChars.y -= UI_HEIGHT - 1; //Subtract 1 more, to let the command prompt onscreen.
	Vec2i_t tResPX = (Vec2i_t){tResChars.x, tResChars.y*2};

	t_createFramebuffer(tResPX);

	for (int x=0; x<tResPX.x; x++) {
		for (int y=0; y<tResPX.y; y++) {
			RGB_t colour = {
				(int)((float)(x) / (float)(tResPX.x) * 255.0f),
				(int)((float)(y) / (float)(tResPX.y) * 255.0f),
				0u
			};
			t_writePX((Vec2i_t){x,y}, colour);
		}
	}

	t_drawFramebuffer();

	t_deleteFramebuffer();

	return 1;
}