/* main.c */

#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <math.h>

#include "src/types.h"
#include "src/terminal.h"


int run = 1;
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


	double start;
	unsigned int frameNumber = 0u;
	do { //Frameloop
		start = now();

		//Example frame-task.
		for (int x=0; x<tResPX.x; x++) {
			for (int y=0; y<tResPX.y; y++) {
				RGB_t colour = {
					(int)((float)(x) / (float)(tResPX.x) * 255.0f),
					(int)((float)(y) / (float)(tResPX.y) * 255.0f),
					(int)((sin((double)(frameNumber) * 1.0e-1) + 1.0) * 128.0)
				};
				t_writePX((Vec2i_t){x,y}, colour);
			}
		}

		t_resetCursor();
		t_drawFramebuffer();

		double elapsed = now() - start;
		double remaining = DT - elapsed;
		if (remaining > 0) {
			struct timespec ts;
			ts.tv_sec = (time_t)(remaining);
			ts.tv_nsec = (long)((remaining - ts.tv_sec) * 1.0e9);
			nanosleep(&ts, NULL);
		}
		frameNumber++;
	} while (run);

	t_deleteFramebuffer();

	return 1;
}