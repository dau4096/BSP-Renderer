/* main.c */

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "../src/types.h"
#include "../src/maths.h"
#include "terminal.h"
#include "../src/io.h"
#include "graphics.h"
#include "../src/file.h"
#include "edit.h"




#define EDITOR_MAX_VERTICES 1024
#define EDITOR_MAX_LINEDEFS 512
#define EDITOR_MAX_SECTORS 128
#define HZ 30.0f
#define DT (1.0f / HZ)



#ifdef DEBUG
#define FB_Y_OFFSET 1
#else
#define FB_Y_OFFSET 0
#endif



double now(void) {
	struct timespec ts;
	timespec_get(&ts, TIME_UTC);
	return ts.tv_sec + (ts.tv_nsec / 1.0e9l);
}





volatile sig_atomic_t run = 1;
void signalHandler(int sig) {
	//^C interrupt caught. Fallback for no IO.
	run = 0;
}





int main(int argc, char* argv[]) {
	signal(SIGINT, signalHandler);

	char* xmlFileName = malloc(64 * sizeof(char));
	if (argc > 1) {
		//Very simple cmdline handling for arguments. Only has XML filepath currently.
		strcpy(xmlFileName, "xml/");
		strcat(xmlFileName, argv[1]);
	} else {
		strcpy(xmlFileName, "xml/tmp.xml");
	}

	Vec2i_t tResChars = t_getTerminalSize();
	tResChars.y -= FB_Y_OFFSET;
	t_createFramebuffer(tResChars); //Create framebuffer. (2D pixel data)


	int ioSuccess = io_init();
	if (!ioSuccess) {
		//Failed to find valid keyboard.
		printf("Failed to find valid keyboard input.\n");
		io_quit();
		t_deleteFramebuffer();
		return -1;
	}


	int loadXMLSuccess = f_loadGeo(xmlFileName);
	if (!loadXMLSuccess) {
		//Failed to read XML file properly; must be blank. Use placeholder buffers.
		printf("Creating blank geo with MAX %dv, %dld, %ds\n", EDITOR_MAX_VERTICES, EDITOR_MAX_LINEDEFS, EDITOR_MAX_SECTORS);
		g_vertices = calloc(EDITOR_MAX_VERTICES, sizeof(Vec2f_t));		g_numVertices = 0u;
		g_lineDefs = calloc(EDITOR_MAX_VERTICES, sizeof(LineDef_t));	g_numLineDefs = 0u;
		g_sectors  = calloc(EDITOR_MAX_VERTICES, sizeof(Sector_t));		g_numSectors  = 0u;
	}

	ed_init(xmlFileName);


	double start;
	double dt = 0.0;
	unsigned int frameNumber = 0u;
	do { //Frameloop
		start = now();
		Vec2i_t newTResChars = t_getTerminalSize();
		newTResChars.y -= FB_Y_OFFSET;
		if ((newTResChars.x != tResChars.x) || (newTResChars.y != tResChars.y)) {
			//Remake framebuffer to fit new res.
			tResChars = newTResChars;
			t_createFramebuffer(tResChars); //Remake framebuffer to the correct resolution.

		} else {
			//No need to clear framebuffer if it was reallocated, calloc automatically clears it to black.
			t_clearFramebuffer();
		}

		io_pollEvents();


		io_handleViewportMovement(DT);
	#ifdef DEBUG
		printf("%f, %f,    %f,       %g\n", r_camera->position.x, r_camera->position.y, r_camera->FOV, dt);
	#endif


	#ifndef SUPPRESS_FRAMEBUFFER_OUTPUT
		r_drawGeo();

		t_resetCursor();
		t_drawFramebuffer();

		t_resetCursor();
	#endif
		ed_update(framebuffer.resolutionCHARS, &xmlFileName);


		dt = now() - start;
		double remaining = DT - dt;
		if (remaining > 0) {
			struct timespec ts;
			ts.tv_sec = (time_t)(remaining);
			ts.tv_nsec = (long)((remaining - ts.tv_sec) * 1.0e9);
			nanosleep(&ts, NULL);
		}

	} while (run && !(keyMapPress[K_QUIT]));
	printf("\n");


	t_deleteFramebuffer();
	io_quit();

	return 1;
}

