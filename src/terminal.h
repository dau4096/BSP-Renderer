/* terminal.h */
#ifndef TERMINAL_H
#define TERMINAL_H

#include "types.h"


//////// DATA ////////
extern Buffer_t framebuffer;
//////// DATA ////////

//////// UTILITY ////////
Vec2i_t t_getTerminalSize(void);
//////// UTILITY ////////


//////// INITIALISATION ////////
void t_createFramebuffer(const Vec2i_t resolutionChars);
RGB_t* t_getFramebufferPTR(void);
void t_deleteFramebuffer(void);
//////// INITIALISATION ////////


//////// DIRECT DRAW ////////
void t_quantise(RGB_t* colour);
void t_writePX(const Vec2i_t position, RGB_t colour);
RGB_t t_readPX(const Vec2i_t position);
//////// DIRECT DRAW ////////


//////// FRAMEBUFFER ////////
void t_resetCursor(void);
void t_drawFramebuffer(void);
void t_clearFramebuffer(void);
void t_fillFramebuffer(const RGB_t colour);
void t_swapBuffers(void);
//////// FRAMEBUFFER ////////


#endif