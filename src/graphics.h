/* graphics.h */
#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "types.h"


//////// DATA ////////
#define MAX_TEXTURES 64u


extern Camera_t* r_camera;


//Placeholder values; replace with calloc() heap stuff later.
extern unsigned int g_numVertices;
extern unsigned int g_numLineDefs;
extern unsigned int g_numSectors;

extern Vec2f_t* g_vertices;
extern LineDef_t* g_lineDefs;
extern Sector_t* g_sectors;
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
int r_loadTextures(const char** textureNames, const unsigned int numTexturePaths);
//////// INITIALISATION ////////



#endif