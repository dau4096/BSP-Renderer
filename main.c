/* main.c */

#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <math.h>

#include "src/types.h"
#include "src/terminal.h"
#include "src/io.h"
#include "src/graphics.h"




volatile sig_atomic_t run = 1;
void signalHandler(int sig) {
	//^C interrupt caught. Temporary fix for no IO.
	run = 0;
}


double now(void) {
	struct timespec ts;
	timespec_get(&ts, TIME_UTC);
	return ts.tv_sec + (ts.tv_nsec / 1.0e9l);
}


#define HZ 60.0f
#define DT (1.0f / HZ)


//Will be used later to add a UI bar at the bottom.
#define UI_HEIGHT 0u



int main(void) {
	signal(SIGINT, signalHandler);

	Vec2i_t tResChars = t_getTerminalSize();
	tResChars.y -= UI_HEIGHT + 1; //Subtract 1 more, to let the command prompt onscreen.
	Vec2i_t tResPX = (Vec2i_t){tResChars.x, tResChars.y*2};

	t_createFramebuffer(tResPX);
	io_init();
	r_initCamera();
	r_createGeometry();


	double start;
	unsigned int frameNumber = 0u;
	do { //Frameloop
		start = now();
		Vec2i_t newTResChars = t_getTerminalSize();
		newTResChars.y -= UI_HEIGHT + 1; //Subtract 1 more, to let the command prompt onscreen.
		if ((newTResChars.x != tResChars.x) || (newTResChars.y != tResChars.y)) {
			//Remake framebuffer to fit new res.
			tResChars = newTResChars;
			Vec2i_t tResPX = (Vec2i_t){tResChars.x, tResChars.y*2};
			t_createFramebuffer(tResPX);
		}
		t_clearFramebuffer();
		io_pollEvents();


		//Tasks for this frame;
		io_handleInputs(&camera);
		r_drawFrame(tResPX);
		

		t_resetCursor();
		t_drawFramebuffer();
		fflush(stdout);



		double elapsed = now() - start;
		printf("Frame %d took: %.1lfms Theoretical FPS: %.1lf", frameNumber, elapsed*1000.0, 1.0 / elapsed); //Display real DT.
		double remaining = DT - elapsed;
		if (remaining > 0) {
			struct timespec ts;
			ts.tv_sec = (time_t)(remaining);
			ts.tv_nsec = (long)((remaining - ts.tv_sec) * 1.0e9);
			nanosleep(&ts, NULL);
		}
		frameNumber++;
	} while (run && !(keyMap[K_QUIT]));
	printf("\n");

	t_deleteFramebuffer();
	io_quit();

	return 1;
}