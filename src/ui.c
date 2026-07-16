/* ui.c */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "graphics.h"
#include "physics.h"


void ui_drawDebugValues(const Vec2i_t resolution, const int uiHeight) {
	//Purely for debugging. No decoration.
	printf( //1st line : Camera information
		"Position: [%.2f, %.2f, %.2f]    Yaw: [%.0f°]    Z-Velocity: [%.2f]\n",
		r_camera->position.x, r_camera->position.y, r_camera->Z,
		r_camera->yaw * 180.0f / M_PI, r_camera->Zvelocity
	);

	Sector_t* currentSector = g_sectors + currentSectorID;
	printf( //2nd line : Sector information
		"Sector: {ID: [%u]    Number Of Linedefs: [%u]    Light-Level: [%u]    Floor: [%.2f]    Ceil: [%.2f]}\n",
		currentSectorID, currentSector->numLineDefs, currentSector->lightLevel,
		currentSector->floorHeight, currentSector->ceilingHeight
	);


	printf( //3rd line : Physics information
		"Floor: [%s]    Portal: [%s]    Prev-Jump: [%s]\n",
		((isOnFloor) ? "TRUE " : "FALSE"), //Seems to alternate every other frame? Investigate.
		((isInPortal) ? "TRUE " : "FALSE"),
		((prevJump) ? "TRUE " : "FALSE")
	);


	unsigned int numSectors = 0u;
	unsigned int numLineDefs = 0u;
	for (Sector_t* thisSector=g_sectors; thisSector<g_sectors+g_numSectors; thisSector++) {if (thisSector->numLineDefs) {numSectors++;}}
	for (LineDef_t* thisLineDef=g_lineDefs; thisLineDef<g_lineDefs+g_numLineDefs; thisLineDef++) {if (thisLineDef->isValid) {numLineDefs++;}}
	printf( //4th line : Graphics information
		"Resolution: [%i, %i]    Sectors: [%u]    LineDefs: [%u]\n",
		resolution.x, resolution.y*2,
		numSectors, numLineDefs
	);
}


void ui_drawDefaultInterface(const int width, const int uiHeight) {
	//Looks nice, displays player data.
	
	//Top of the frame
	printf("┏");
	for (unsigned int i=0u; i<width-2u; i++) {printf("━");}
	printf("┓\n");


	//UI contents;
	//[PLACEHOLDER] Blank UI with borders.
	char* line = malloc(sizeof(char) * (width - 1u));
	for (unsigned int i=0u; i<width-2u; i++) {line[i] = ' ';}
	for (unsigned int i=1u; i<uiHeight-1u; i++) {printf("┃%s┃\n", line);}
	free(line);



	//Bottom of the frame
	printf("┗");
	for (unsigned int i=0u; i<width-2u; i++) {printf("━");}
	printf("┛\n");
}



void ui_drawInterface(const Vec2i_t resolution, const int uiHeight) {
#ifdef DEBUG_VALUES
	//Don't draw decorative frame around UI, when debugging.
	ui_drawDebugValues(resolution, uiHeight);
#else
	//Draw nice looking UTF8 frame around UI.
	ui_drawDefaultInterface(resolution.x, uiHeight);
#endif
}
