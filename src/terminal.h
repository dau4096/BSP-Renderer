/* terminal.h */
#ifndef TERMINAL_H
#define TERMINAL_H


#include "types.h"


extern Buffer_t framebuffer;


Vec2i_t t_getTerminalSize(void);

void t_createFramebuffer(Vec2i_t resolution);
RGB_t* t_getFramebufferPTR(void);
void t_deleteFramebuffer(void);

void t_writePX(Vec2i_t position, RGB_t colour);
RGB_t t_readPX(Vec2i_t position);

void t_resetCursor(void);
void t_drawFramebuffer(void);
void t_clearFramebuffer(void);
void t_fillFramebuffer(RGB_t colour);


#endif