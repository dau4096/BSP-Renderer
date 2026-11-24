/* C-Header.h */
#ifndef DISPLAY_H
#define DISPLAY_H


#include "types.h"


//////// DATA ////////
typedef enum {
    DISPLAY_UNKNOWN,
    DISPLAY_WIN32, //Used as a "Do not use" option that is not UNKNOWN.
    DISPLAY_WAYLAND,
    DISPLAY_X11
} DisplayBackend;

extern int isUsingDirectDisplay;
//////// DATA ////////



//////// DISPLAY ////////
int d_init(void);	 		//Initialises the display manager.
int d_isRunning(void); 		//Checks if display manager is running.
int d_shutdown(void);	 	//Shuts down the display manager.

int d_createWindow(const Vec2i_t resolution, const char* title); //Creates a window in this given display manager.
int d_destroyWindow(void);	//Kills the display manager's window.

int d_pollWindow(void);	 	//Polls the display manager's window.
int d_updateWindow(void);	//Updates the display manager's window.
int d_resizeWindow(const Vec2i_t resolution); //Resizes the display manager's window.
//////// DISPLAY ////////



#endif
