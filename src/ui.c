/* ui.c */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "io.h"
#include "graphics.h"
#include "physics.h"




#ifdef DEBUG_VALUES
//Shows generic debug values instead of a UI.
unsigned int UI_HEIGHT = 4u;

#else
//Will be used later to add a UI bar at the bottom.
unsigned int UI_HEIGHT = 5u;
#endif



unsigned int ui_getStringLength(const char* string) {
	unsigned int i = 0u;
	while (string[i] != '\0') {i++;}
	return i;
}



void ui_drawDebugValues(const Vec2i_t resolution, const int uiHeight, char** buf, unsigned int* size) {
	//Purely for debugging. No decoration.
	char* bufPTR = *buf;
	sprintf( //1st line : Camera information
		bufPTR,
		"Position: [%.2f, %.2f, %.2f]    Yaw: [%.0f°]    Z-Velocity: [%.2f]\n",
		r_camera->position.x, r_camera->position.y, r_camera->Z,
		r_camera->yaw * 180.0f / M_PI, r_camera->Zvelocity
	);
	bufPTR = *buf + ui_getStringLength(*buf);

	Sector_t* currentSector = g_sectors + currentSectorID;
	sprintf( //2nd line : Sector information
		bufPTR,
		"Sector: {ID: [%u]    Number Of Linedefs: [%u]    Light-Level: [%u]    Floor: [%.2f]    Ceil: [%.2f]}\n",
		currentSectorID, currentSector->numLineDefs, currentSector->lightLevel,
		currentSector->floorHeight, currentSector->ceilingHeight
	);
	bufPTR = *buf + ui_getStringLength(*buf);


#ifdef DEBUG_DRAW_ORDER
	sprintf( //3rd line : Physics information & Debug
		bufPTR,
		"Floor: [%s]    Drawn-LDs: [%u]\n",
		((isOnFloor) ? "TRUE " : "FALSE"),
		g_numLineDefs-currentDrawNumber
	);
	bufPTR = *buf + ui_getStringLength(*buf);
#else
	sprintf( //3rd line : Physics information
		bufPTR,
		"Floor: [%s]\n",
		((isOnFloor) ? "TRUE " : "FALSE")
	);
	bufPTR = *buf + ui_getStringLength(*buf);
#endif


	sprintf( //4th line : Graphics information
		bufPTR,
		"Resolution: [%i, %i]    Sectors: [%u]    LineDefs: [%u]\n",
		resolution.x, resolution.y*2,
		g_numSectors, g_numLineDefs
	);
	*size = ui_getStringLength(*buf);
}


void ui_drawDefaultInterface(const int width, const int uiHeight, char** buf, unsigned int* size) {
	//Looks nice, displays player data.
	
	//Top of the frame
	char* bufPTR = *buf;
	sprintf(bufPTR, "┏"); bufPTR++;
	for (unsigned int i=0u; i<width-2u; i++) {sprintf(bufPTR, "━"); bufPTR++;}
	sprintf(bufPTR, "┓\n"); bufPTR++;


	//UI contents;
	//[PLACEHOLDER] Blank UI with borders.
	char* line = malloc(sizeof(char) * (width - 1u));
	for (unsigned int i=0u; i<width-2u; i++) {line[i] = ' ';}
	for (unsigned int i=1u; i<uiHeight-1u; i++) {sprintf(bufPTR, "┃%s┃\n", line); bufPTR = *buf + ui_getStringLength(*buf);}
	free(line);



	//Bottom of the frame
	sprintf(bufPTR, "┗"); bufPTR++;
	for (unsigned int i=0u; i<width-2u; i++) {sprintf(bufPTR, "━"); bufPTR++;}
	sprintf(bufPTR, "┛\n"); bufPTR++;

	*size = ui_getStringLength(*buf);
}



void ui_getInterface(const Vec2i_t resolution, const int uiHeight, char** buf, unsigned int* size) {
#ifdef DEBUG_VALUES
	//Don't draw decorative frame around UI, when debugging.
	ui_drawDebugValues(resolution, uiHeight, buf, size);
#else
	//Draw nice looking UTF8 frame around UI.
	ui_drawDefaultInterface(resolution.x, uiHeight, buf, size);
#endif	
}



void ui_drawInterface(const Vec2i_t resolution, const int uiHeight) {
	char* uiBuffer = calloc(uiHeight * resolution.x, sizeof(char));
	unsigned int uiBufferSize;
	ui_getInterface(resolution, uiHeight, &uiBuffer, &uiBufferSize);
	printf("%s", uiBuffer);
}






void ui_getHTML(const Vec2i_t resolution, const int uiHeight, char** buf, unsigned int* size) {
	char* uiBuffer = calloc(uiHeight * resolution.x, sizeof(char));
	unsigned int uiBufferSize;
	ui_getInterface(resolution, uiHeight, &uiBuffer, &uiBufferSize);
	*size = uiBufferSize + 11u;
	*buf = calloc(*size, sizeof(char));
	sprintf(*buf, "<pre>%s</pre>", uiBuffer);
	free(uiBuffer);
}
