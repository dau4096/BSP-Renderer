/* graphics.h */
#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "types.h"


extern Camera_t camera;



int r_getCentreX(const Vec2f_t position, const Vec2i_t resolution);

void r_drawFrame(const Vec2i_t resolution);

void r_initCamera(void);
void r_createGeometry(void);


#endif