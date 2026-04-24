/* graphics.h */
#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "types.h"


//////// DATA ////////
extern Camera_t camera;
//////// DATA ////////


//////// DEPTH MAPPING ////////
void r_reallocColumnBuffers(const int width);
//////// DEPTH MAPPING ////////


//////// DRAWING ////////
int r_getCentreX(const Vec2f_t position, const Vec2i_t resolution);

void r_drawFrame(const Vec2i_t resolution);
//////// DRAWING ////////


//////// INITIALISATION ////////
void r_initCamera(void);
int r_createGeometry(void);
void r_freeBSPTree(void);
//////// INITIALISATION ////////


#endif