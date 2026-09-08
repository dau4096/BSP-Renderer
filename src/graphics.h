/* graphics.h */
#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "types.h"


//////// DATA ////////
extern Camera_t* r_camera;


//Placeholder values; replace with calloc() heap stuff later.
extern unsigned int g_numVertices;
extern unsigned int g_numLineDefs;
extern unsigned int g_numSectors;

extern Vec2f_t* g_vertices;
extern LineDef_t* g_lineDefs;
extern Sector_t* g_sectors;
//////// DATA ////////


//////// DRAWING ////////
int r_getCentreX(const Vec2f_t position);
float r_getLineDefDistance(const LineDef_t* thisLineDef, const Vec2f_t position);
void r_drawFrame(void);
//////// DRAWING ////////


//////// INITIALISATION ////////
void r_initCamera(void);
//////// INITIALISATION ////////



#endif