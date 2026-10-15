/* graphics.h */
#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "../src/types.h"


//////// DATA ////////
#define MAX_TEXTURES 64u
#define FALLBACK_TEXTURE_PATH "textures/fallback.png"
extern unsigned int fallbackTextureIndex;


extern Camera_t* r_camera;


extern unsigned int g_numVertices;
extern unsigned int g_numLineDefs;
extern unsigned int g_numSectors;

extern Vec2f_t* g_vertices;
extern LineDef_t* g_lineDefs;
extern Sector_t* g_sectors;
//////// DATA ////////


//////// DRAWING ////////
void r_drawLineAuto(const Vec2i_t start, const Vec2i_t end, const RGB_t colour);

void r_drawGeo(void);
//////// DRAWING ////////


#endif