/* main.c */

#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <pthread.h>

#include "src/types.h"
#include "src/io.h"
#include "src/loader.h"
#include "src/terminal.h"
#include "src/graphics.h"
#include "src/ui.h"
#include "src/multithread.h"
#include "src/physics.h"






volatile sig_atomic_t run = 1;
void signalHandler(int sig) {
	//^C interrupt caught. Fallback for no IO.
	run = 0;
}


double now(void) {
	struct timespec ts;
	timespec_get(&ts, TIME_UTC);
	return ts.tv_sec + (ts.tv_nsec / 1.0e9l);
}


#ifdef LIMITED_FREQ
#define HZ 60.0f
#define DT (1.0f / HZ)
#endif



int main(int argc, char* argv[]) {
	signal(SIGINT, signalHandler);


	char xmlFileName[64];
	if (argc > 1) {
		//Very simple cmdline handling for arguments. Only has XML filepath currently.
		strcpy(xmlFileName, "xml/");
		strcat(xmlFileName, argv[1]);
	} else {
		strcpy(xmlFileName, "xml/doom.xml");
	}


	Vec2i_t tResChars = t_getTerminalSize();
	tResChars.y -= UI_HEIGHT + 2u; //Subtract 1 more, to let the command prompt onscreen.
	t_createFramebuffer(tResChars); //Create framebuffer. (2D pixel data)
	r_reallocColumnBuffers(); //Create depthmap. (1D depth data)

	int ioSuccess = io_init();
	if (!ioSuccess) {
		//Failed to find valid keyboard.
		printf("Failed to find valid keyboard input.\n");
		io_quit();
		t_deleteFramebuffer();
		return -1;
	}


	r_initCamera();
	int loadXMLSuccess = l_loadGeo(xmlFileName);
	if (!loadXMLSuccess) {
		//Failed to read an XML file properly
		printf("Failed to read XML file.\n");
		io_quit();
		t_deleteFramebuffer();
		return -1;
	}


	pthread_t terminalDrawThread;
	if (!mt_createThread(mt_framebufferDrawLoop, &terminalDrawThread)) {
		printf("Failed to create terminal output thread.");
		return -1;
	}


	double start;
	double dt = 0.0;
	unsigned int frameNumber = 0u;
	do { //Frameloop
		start = now();
		Vec2i_t newTResChars = t_getTerminalSize();
		newTResChars.y -= UI_HEIGHT + 2u; //Subtract 1 more, to let the command prompt onscreen.
		if ((newTResChars.x != tResChars.x) || (newTResChars.y != tResChars.y)) {
			//Remake framebuffer to fit new res.
			tResChars = newTResChars;
			t_createFramebuffer(tResChars); //Remake framebuffer to the correct resolution.
			r_reallocColumnBuffers(); //Reallocate depthmap to the correct width.

		} else {
			//No need to clear framebuffer if it was reallocated, calloc automatically clears it to black.
			t_clearFramebuffer();
		}

		io_pollEvents();


		//Tasks for this frame;
	#ifdef LIMITED_FREQ
		p_updateCamera(r_camera, DT);
	#else
		p_updateCamera(r_camera, dt);
	#endif

		//Render frame
		r_drawFrame();
		
	#ifndef SUPPRESS_FRAMEBUFFER_OUTPUT
		mt_setDrawReady();
	#endif
		fflush(stdout);


		dt = now() - start;
		//printf("Frame %d took: %.3lfms Theoretical FPS: ~%.0lf", frameNumber, dt*1000.0, 1.0 / dt); //Display real DT. //Can't be shown ATM due to multithreading. !FIX!
	#ifdef SUPPRESS_FRAMEBUFFER_OUTPUT
		printf("\n");
	#endif

	#ifdef LIMITED_FREQ
		double remaining = DT - dt;
		if (remaining > 0) {
			struct timespec ts;
			ts.tv_sec = (time_t)(remaining);
			ts.tv_nsec = (long)((remaining - ts.tv_sec) * 1.0e9);
			nanosleep(&ts, NULL);
		}
	#endif
		frameNumber++;

	#ifdef SUPPRESS_FRAMEBUFFER_OUTPUT
		printf("\n");
	#endif
	} while (run && !(keyMapPress[K_QUIT]));
	printf("\n");



	mt_terminateThread(terminalDrawThread);
	t_deleteFramebuffer();
	io_quit();

	return 1;
}