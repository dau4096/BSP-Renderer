/* main.c */

#include <stdio.h>

#include "src/types.h"
#include "src/terminal.h"


int main(void) {

	t_createFramebuffer((Vec2i_t){64, 32});

	for (int x=0; x<64; x++) {
		for (int y=0; y<32; y++) {
			RGB_t colour = {x*4, y*8, 0u};
			t_writePX((Vec2i_t){x,y}, colour);
		}
	}

	t_drawFramebuffer();

	t_deleteFramebuffer();

	return 1;
}