/* loader.c */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fxcg/heap.h>

#include "types.h"
#include "graphics.h" //For the geometry datasets





int l_loadGeo(void) {
	//Loads some file into the geometry datasets provided.
	//Returns success

	//Deallocate if already filled.
	if (g_vertices) {free(g_vertices);}
	if (g_lineDefs) {free(g_lineDefs);}
	if (g_sectors) {free(g_sectors);}


	g_numVertices = 0u;
	g_vertices = sys_calloc(sizeof(Vec2f_t), g_numVertices);
	g_numLineDefs = 0u;
	g_vertices = sys_calloc(sizeof(LineDef_t), g_numLineDefs);
	g_numSectors = 0u;
	g_vertices = sys_calloc(sizeof(Sector_t), g_numSectors);


	return TRUE;
}



