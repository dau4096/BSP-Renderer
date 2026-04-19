/* terminal.h */
#ifndef TERMINAL_H
#define TERMINAL_H


#include "types.h"


extern Buffer_t framebuffer;


Vec2i_t t_getTerminalSize();

void t_createFramebuffer(Vec2i_t resolution);
void t_deleteFramebuffer();

void t_writePX(Vec2i_t position, RGB_t colour);
RGB_t t_readPX(Vec2i_t position);

void t_drawFramebuffer();


#endif