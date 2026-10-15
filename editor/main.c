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
#include "../src/file.h"
#include "graphics.h"




#define EDITOR_MAX_VERTICES 1024
#define EDITOR_MAX_LINEDEFS 512
#define EDITOR_MAX_SECTORS 128
#define MOVEMENT_SPEED_BASE 1000.0f /* In units/second */
#define ZOOM_SPEED 50.0f
#define HZ 30.0f
#define DT (1.0f / HZ)


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



void io_handleViewportMovement(double dt);



int main(int argc, char* argv[]) {
	signal(SIGINT, signalHandler);

	char xmlFileName[64];
	if (argc > 1) {
		//Very simple cmdline handling for arguments. Only has XML filepath currently.
		strcpy(xmlFileName, "xml/");
		strcat(xmlFileName, argv[1]);
	} else {
		strcpy(xmlFileName, "xml/tmp.xml");
	}

	Vec2i_t tResChars = t_getTerminalSize();
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
		g_vertices = calloc(EDITOR_MAX_VERTICES, sizeof(Vec2f_t));		g_numVertices = EDITOR_MAX_VERTICES;
		g_lineDefs = calloc(EDITOR_MAX_VERTICES, sizeof(LineDef_t));	g_numLineDefs = EDITOR_MAX_LINEDEFS;
		g_sectors  = calloc(EDITOR_MAX_VERTICES, sizeof(Sector_t));		g_numSectors  = EDITOR_MAX_SECTORS;
	}


	double start;
	double dt = 0.0;
	unsigned int frameNumber = 0u;
	do { //Frameloop
		start = now();
		Vec2i_t newTResChars = t_getTerminalSize();
		if ((newTResChars.x != tResChars.x) || (newTResChars.y != tResChars.y)) {
			//Remake framebuffer to fit new res.
			tResChars = newTResChars;
			t_createFramebuffer(tResChars); //Remake framebuffer to the correct resolution.

		} else {
			//No need to clear framebuffer if it was reallocated, calloc automatically clears it to black.
			t_clearFramebuffer();
		}

		io_pollEvents();


		io_handleViewportMovement(dt);
		printf("%f, %f,    %f\n", r_camera->position.x, r_camera->position.y, r_camera->FOV);


	#ifndef SUPPRESS_FRAMEBUFFER_OUTPUT
		r_drawGeo();

		t_resetCursor();
		t_drawFramebuffer();
	#endif

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




void io_handleViewportMovement(double dt) {
	float movSpeed = MOVEMENT_SPEED_BASE * dt * r_camera->FOV;
	if (keyMapHold[K_MOVE_FORE]) {r_camera->position = v2f_add(r_camera->position, (Vec2f_t){.x=0.0f, .y=movSpeed});}
	if (keyMapHold[K_MOVE_BACK]) {r_camera->position = v2f_sub(r_camera->position, (Vec2f_t){.x=0.0f, .y=movSpeed});}
	if (keyMapHold[K_MOVE_LEFT]) {r_camera->position = v2f_sub(r_camera->position, (Vec2f_t){.x=movSpeed, .y=0.0f});}
	if (keyMapHold[K_MOVE_RIGHT]) {r_camera->position = v2f_add(r_camera->position, (Vec2f_t){.x=movSpeed, .y=0.0f});}

	float zoomSpeed = 1.0f + (ZOOM_SPEED * dt);
	if (keyMapHold[K_MOVE_FAST]) {r_camera->FOV *= zoomSpeed;}
	if (keyMapHold[K_MOVE_SLOW]) {r_camera->FOV /= zoomSpeed;}
}