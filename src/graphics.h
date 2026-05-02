/* graphics.h */
#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "types.h"


//////// DATA ////////
extern Camera_t camera;



//Placeholder values; replace with calloc() heap stuff later.
#define MAX_VERTICES 64u
#define MAX_LINEDEFS 32u
#define MAX_SECTORS  16u

extern Vec2f_t g_vertices[MAX_VERTICES];
extern LineDef_t g_lineDefs[MAX_LINEDEFS];
extern Sector_t g_sectors[MAX_SECTORS];
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
int r_loadTextures(void);
void r_createGeometry(void);
//////// INITIALISATION ////////



#endif