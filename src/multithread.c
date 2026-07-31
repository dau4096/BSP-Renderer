/* multithread.c */

#include <pthread.h>
#include <stdatomic.h>

#include "multithread.h"
#include "terminal.h"
#include "ui.h"


//////// MULTITHREADING ////////
int mt_createThread(void* (*threadFunction)(void*), pthread_t* thisThread) {
	//Takes some func, that needs no args and returns a success/faliure value, and creates a thread for it.
	int rc = pthread_create(thisThread, NULL, threadFunction, NULL);
	if (rc != 0) {
		//Faliure to create thread.
		return FALSE;
	}

	return TRUE;
}


int mt_waitForThread(const pthread_t thisThread) {
	//Joins to thread, and blocks exec until said thread finishes.
	int rc = pthread_join(thisThread, NULL);
	if (rc != 0) {return FALSE; /* Faliure */}
	return TRUE;
}


int mt_terminateThread(const pthread_t thisThread) {
	//Requests a thread terminate.
	//!![May not be respected]!!
	int rc = pthread_cancel(thisThread); //Tell thread to end
	if (rc != 0) {return FALSE; /* Faliure */}
	mt_waitForThread(thisThread); //Wait for it to exit before returning.
	return TRUE;
}


pthread_t mt_getCurrentThreadID(void) {
	//Gets this current thread's ID.
	return pthread_self();
}


void mt_exit(void) {
	//Forcefully closes this thread.
	pthread_exit(NULL);
}
//////// MULTITHREADING ////////



//////// TERMINAL RENDERING ////////
pthread_mutex_t framebufferMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t framebufferReady = PTHREAD_COND_INITIALIZER;
pthread_cond_t framebufferDrawn = PTHREAD_COND_INITIALIZER;

int frameReady = FALSE;
int drawFinished = TRUE;



void mt_setDrawReady(void) {
	//Sets the atomic to signal a new draw is permitted.
	pthread_mutex_lock(&framebufferMutex);

	//Wait until the terminal drawing thread has finished with the previous frame.
	while (!drawFinished) {pthread_cond_wait(&framebufferDrawn, &framebufferMutex);}

	t_swapBuffers(); //Swaps framebuffer front/back.

	drawFinished = FALSE;
	frameReady = TRUE;

	pthread_cond_signal(&framebufferReady);
	pthread_mutex_unlock(&framebufferMutex);
}


void* mt_framebufferDrawLoop(void* arg) {
	while (TRUE) {
		pthread_mutex_lock(&framebufferMutex);


		//Wait until the frame is ready to be displayed.
		while (!frameReady) {pthread_cond_wait(&framebufferReady, &framebufferMutex);}

		frameReady = FALSE;

		pthread_mutex_unlock(&framebufferMutex);


		t_resetCursor(); //Resets cursor using ANSI escape codes.
		t_drawFramebuffer(); //Draw the frame to the terminal.

	#ifndef SUPPRESS_INTERFACE_OUTPUT
		ui_drawInterface(framebuffer.resolutionCHARS, UI_HEIGHT);
	#endif


		pthread_mutex_lock(&framebufferMutex);

		drawFinished = TRUE;

		pthread_cond_signal(&framebufferDrawn);
		pthread_mutex_unlock(&framebufferMutex);
	}

	return MT_SUCCESS;
}
//////// TERMINAL RENDERING ////////