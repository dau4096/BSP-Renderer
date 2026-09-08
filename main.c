/* main.c */
#include <fxcg/display.h>

#include <string.h>


#include "src/types.h"
#include "src/terminal.h"
#include "src/io.h"
#include "src/loader.h"
#include "src/graphics.h"
#include "src/physics.h"
#include "src/ui.h"




#ifdef DEBUG_VALUES
//Shows generic debug values instead of a UI.
#define UI_HEIGHT 4u

#else
//Will be used later to add a UI bar at the bottom.
#define UI_HEIGHT 5u
#endif


int main(void) {
	Bdisp_EnableColor(1); //Use RGB565 colour.
	t_drawFramebuffer();

	t_createFramebuffer(); //Create framebuffer. (2D pixel data)


	r_initCamera();
	int loadGeoSuccess = l_loadGeo();
	if (!loadGeoSuccess) {
		//Failed to read an XML file properly
		t_deleteFramebuffer();
		return -1;
	}




	unsigned int frameNumber = 0u;
	int RUN = TRUE;
	do { //Frameloop
		io_pollEvents(&RUN);


		//Tasks for this frame;
		p_updateCamera(r_camera);

		//Render frame
		r_drawFrame();
		
		t_drawFramebuffer();


		frameNumber++;
	} while (RUN && !(keyMapPress[K_QUIT]));

	t_deleteFramebuffer();

	return 0;
}