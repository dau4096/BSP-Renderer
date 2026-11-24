/* display.c */

#include <stdlib.h>
#include <string.h>

#include "types.h"
#include "terminal.h"
#include "display.h"

//////// DATA ////////
static DisplayBackend d_backend = DISPLAY_UNKNOWN;
static int isUsingDirectDisplay = FALSE; //Should use this set of functions?
//////// DATA ////////


//////// WAYLAND ////////
int d_initWayland(void) {
	//TBA
	return TRUE;
}


int d_createWindowWayland(const Vec2i_t resolution, const char* title) {
	//TBA
	return TRUE;
}


int d_pollWindowWayland(void) {
	//TBA
	return TRUE;
}


int d_updateWindowWayland(void) {
	//TBA
	RGB_t* t_fbPTR = t_getFramebufferPTR();
	return TRUE;
}


int d_resizeWindowWayland(const Vec2i_t resolution) {
	//TBA
	return TRUE;
}


int d_destroyWindowWayland(void) {
	//TBA
	return TRUE;
}


int d_shutdownWayland(void) {
	//TBA
	return TRUE;
}
//////// WAYLAND ////////






//////// X11 ////////
int d_initX11(void) {
	//TBA
	return TRUE;
}


int d_createWindowX11(const Vec2i_t resolution, const char* title) {
	//TBA
	return TRUE;
}


int d_pollWindowX11(void) {
	//TBA
	return TRUE;
}


int d_updateWindowX11(void) {
	//TBA
	RGB_t* t_fbPTR = t_getFramebufferPTR();
	return TRUE;
}


int d_resizeWindowX11(const Vec2i_t resolution) {
	//TBA
	return TRUE;
}


int d_destroyWindowX11(void) {
	//TBA
	return TRUE;
}


int d_shutdownX11(void) {
	//TBA
	return TRUE;
}
//////// X11 ////////






//////// EXTERN ////////
#ifndef _WIN32 //Linux only!
DisplayBackend _getBackendHLPR(void) {
	//Finds the current display backend (Wayland/X11)
	const char *session = getenv("XDG_SESSION_TYPE");

	if (session) {
		if (strcmp(session, "wayland") == 0) {return DISPLAY_WAYLAND;}
		if (strcmp(session, "x11") == 0) {return DISPLAY_X11;}
	}

	if (getenv("WAYLAND_DISPLAY")) {return DISPLAY_WAYLAND;}
	if (getenv("DISPLAY")) {return DISPLAY_X11;}
	
	return DISPLAY_UNKNOWN;	
}

DisplayBackend d_getBackend(void) {
	d_backend = _getBackendHLPR();
	return d_backend;
}


#else //Add dummy func that does nothing, because windows doesn't support this entire system of window control. (?)
DisplayBackend d_getBackend(void) {return DISPLAY_WIN32;}
#endif





#define SELECT_MGR(WAYL, X11, ...) \
	switch (d_backend) { \
		case DISPLAY_WAYLAND: {return WAYL(__VA_ARGS__);} \
		case DISPLAY_X11: {return X11(__VA_ARGS__);} \
		case DISPLAY_WIN32: {return FALSE;} \
		case DISPLAY_UNKNOWN: default: {return FALSE;} \
	}

int d_init(void) { //Initialises the display manager.
	d_backend = _getBackendHLPR(); //Initialise the backend enum.
	SELECT_MGR(d_initWayland, d_initX11);
} 
int d_createWindow(const Vec2i_t resolution, const char* title) {SELECT_MGR(d_createWindowWayland, d_createWindowX11, resolution, title);} //Creates a window in this given display manager.
int d_pollWindow(void) {SELECT_MGR(d_pollWindowWayland, d_pollWindowX11);} //Polls the display manager's window.
int d_updateWindow(void) {SELECT_MGR(d_updateWindowWayland, d_updateWindowX11);} //Updates the display manager's window.
int d_resizeWindow(const Vec2i_t resolution) {SELECT_MGR(d_resizeWindowWayland, d_resizeWindowX11, resolution);} //Resizes the display manager's window.
int d_destroyWindow(void) {SELECT_MGR(d_destroyWindowWayland, d_destroyWindowX11);} //Kills the display manager's window.
int d_shutdown(void) {SELECT_MGR(d_shutdownWayland, d_shutdownX11);} //Shuts down the display manager.
//////// EXTERN ////////

