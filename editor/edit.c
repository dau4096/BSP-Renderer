/* edit.c */
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "../src/types.h"
#include "../src/maths.h"
#include "../src/io.h"
#include "../src/file.h"
#include "edit.h"
#include "graphics.h"
#include "menu.h"


//////// CAMERA CTRL ////////
void io_handleViewportMovement(double dt) {
	if (isEditingEntry) {return;}
	float movSpeed = MOVEMENT_SPEED_BASE * r_camera->FOV * dt;
	if (keyMapHold[K_MOVE_FORE]) {r_camera->position = v2f_add(r_camera->position, (Vec2f_t){.x=0.0f, .y=movSpeed});}
	if (keyMapHold[K_MOVE_BACK]) {r_camera->position = v2f_sub(r_camera->position, (Vec2f_t){.x=0.0f, .y=movSpeed});}
	if (keyMapHold[K_MOVE_LEFT]) {r_camera->position = v2f_sub(r_camera->position, (Vec2f_t){.x=movSpeed, .y=0.0f});}
	if (keyMapHold[K_MOVE_RIGHT]) {r_camera->position = v2f_add(r_camera->position, (Vec2f_t){.x=movSpeed, .y=0.0f});}

	float zoomSpeed = 1.0f + (ZOOM_SPEED * dt);
	if (keyMapHold[K_MOVE_FAST]) {r_camera->FOV *= zoomSpeed;}
	if (keyMapHold[K_MOVE_SLOW]) {r_camera->FOV /= zoomSpeed;}
}
//////// CAMERA CTRL ////////


//////// GEOMETRY EDITING ////////
EditMode_e currentEditMode = EDIT_NONE;
unsigned int currentlySelectedID = 0u;


Menu_t editMenus[NUM_EDIT_MODES];


void ed_init(const char* filePath) {
	//Create all of the vertices controls
	Menu_t* saveloadMenu = editMenus + EDIT_NONE;
	mn_createNewMenu("Save/Load", saveloadMenu);
	mn_addSeperator(saveloadMenu, "");
	mn_addStringEntry(saveloadMenu, "Path", filePath);
	mn_addBoolEntry(saveloadMenu, "Save", FALSE); //Replace with some form of func/button ver.
	mn_addBoolEntry(saveloadMenu, "Load", FALSE); //Replace with some form of func/button ver.
	mn_addBoolEntry(saveloadMenu, "Clear", FALSE); //Replace with some form of func/button ver.




	Menu_t* verticesMenu = editMenus + EDIT_VERTICES;
	mn_createNewMenu("Vertices", verticesMenu);
	mn_addSeperator(verticesMenu, "");
	mn_addUIntEntry(verticesMenu, "Vertex ID", 0u);
	mn_addSeperator(verticesMenu, "Position");

	float X, Y;
	if (g_numVertices > 0u) {
		mn_addFloatEntry(verticesMenu, "X", g_vertices->x);
		mn_addFloatEntry(verticesMenu, "Y", g_vertices->y);
	} else {
		mn_addFloatEntry(verticesMenu, "X", 0.0f);
		mn_addFloatEntry(verticesMenu, "Y", 0.0f);
	}




	Menu_t* linedefsMenu = editMenus + EDIT_LINEDEFS;
	mn_createNewMenu("LineDefs", linedefsMenu);
	mn_addSeperator(linedefsMenu, "");
	mn_addUIntEntry(linedefsMenu, "LineDef ID", 0u);
	mn_addSeperator(linedefsMenu, "Geometry");
	if (g_numLineDefs > 0u) {
		mn_addUIntEntry(linedefsMenu, "Start vID", g_lineDefs->vStart);
		mn_addUIntEntry(linedefsMenu, "End vID", g_lineDefs->vEnd);

		mn_addIntEntry(linedefsMenu, "Front sID", g_lineDefs->frontSector);
		mn_addIntEntry(linedefsMenu, "Back sID", g_lineDefs->backSector);

		mn_addSeperator(linedefsMenu, "Visual");
		mn_addStringEntry(linedefsMenu, "Texture", textureNames[g_lineDefs->texture]);
	} else {
		mn_addUIntEntry(linedefsMenu, "Start vID", 0u);
		mn_addUIntEntry(linedefsMenu, "End vID", 0u);

		mn_addIntEntry(linedefsMenu, "Front sID", 0);
		mn_addIntEntry(linedefsMenu, "Back sID", -1);

		mn_addSeperator(linedefsMenu, "Visual");
		mn_addStringEntry(linedefsMenu, "Texture", "textures/fallback.png");
	}



	Menu_t* sectorsMenu = editMenus + EDIT_SECTORS;
	mn_createNewMenu("Sectors", sectorsMenu);
	mn_addSeperator(sectorsMenu, "");
	mn_addUIntEntry(sectorsMenu, "Sector ID", 0u);
	mn_addSeperator(sectorsMenu, "Geometry");
	if (g_numSectors > 0u) {
		mn_addUIntEntry(sectorsMenu, "Light Level", g_sectors->lightLevel);
		
		mn_addSeperator(sectorsMenu, "Floor");
		mn_addFloatEntry(sectorsMenu, "Height", g_sectors->floorHeight);
		mn_addBoolEntry(sectorsMenu, "Use Texture", TRUE);
		if (g_sectors->flags & 0x1u > 0u)  {
			mn_addStringEntry(sectorsMenu, "Texture", textureNames[g_sectors->floorTexture]);
		} else {
			mn_addUIntEntry(sectorsMenu, "R", g_sectors->floorColour.r);
			mn_addUIntEntry(sectorsMenu, "G", g_sectors->floorColour.g);
			mn_addUIntEntry(sectorsMenu, "B", g_sectors->floorColour.b);
		}

		mn_addSeperator(sectorsMenu, "Ceiling");
		mn_addFloatEntry(sectorsMenu, "Height", g_sectors->ceilingHeight);
		mn_addBoolEntry(sectorsMenu, "Use Texture", TRUE);
		if (g_sectors->flags & 0x2u > 0u)  {
			mn_addStringEntry(sectorsMenu, "Texture", textureNames[g_sectors->ceilingTexture]);
		} else {
			mn_addUIntEntry(sectorsMenu, "R", g_sectors->ceilingColour.r);
			mn_addUIntEntry(sectorsMenu, "G", g_sectors->ceilingColour.g);
			mn_addUIntEntry(sectorsMenu, "B", g_sectors->ceilingColour.b);			
		}

		mn_addUIntEntry(sectorsMenu, "#LineDefs", g_sectors->numLineDefs);
		mn_addSeperator(sectorsMenu, "LineDefs");
		char title[16u];
		for (unsigned int i=0u; i<g_sectors->numLineDefs; i++) {
			sprintf(title, "LD[%d]", i);
			mn_addUIntEntry(sectorsMenu, title, g_sectors->lineDefs[i]);
		}

	} else {
		mn_addUIntEntry(sectorsMenu, "Light Level", 0xFF);

		mn_addSeperator(sectorsMenu, "Floor");
		mn_addFloatEntry(sectorsMenu, "Height", 0.0f);
		mn_addBoolEntry(sectorsMenu, "Use Texture", TRUE);
		mn_addStringEntry(sectorsMenu, "Texture", "textures/fallback.png");

		mn_addSeperator(sectorsMenu, "Ceiling");
		mn_addFloatEntry(sectorsMenu, "Height", 0.0f);
		mn_addBoolEntry(sectorsMenu, "Use Texture", TRUE);
		mn_addStringEntry(sectorsMenu, "Texture", "textures/fallback.png");

		mn_addUIntEntry(sectorsMenu, "#LineDefs", 0u);
		mn_addSeperator(sectorsMenu, "LineDefs");
	}
}


#define EQU(X, Y) (strcmp((X), (Y)) == 0u)
#define ENTRY_IS(Y) (EQU(changedEntry->title, (Y)))
void ed_update(const Vec2i_t resolutionChars, char** filePath) {
	if (!isEditingEntry && keyMapPress[K_TURN_LEFT])  {currentEditMode = (currentEditMode-1) % NUM_EDIT_MODES;}
	if (!isEditingEntry && keyMapPress[K_TURN_RIGHT]) {currentEditMode = (currentEditMode+1) % NUM_EDIT_MODES;}

	Menu_t* currentMenu = editMenus + currentEditMode;
	mn_editEntry(currentMenu);
	if (!isEditingEntry && keyMapPress[K_NAV_UP])   {currentMenu->selectedEntry = (currentMenu->selectedEntry-1) % currentMenu->numEntries;}
	if (!isEditingEntry && keyMapPress[K_NAV_DOWN]) {currentMenu->selectedEntry = (currentMenu->selectedEntry+1) % currentMenu->numEntries;}

#ifndef SUPPRESS_FRAMEBUFFER_OUTPUT
	mn_draw(currentMenu, resolutionChars);
#endif

	if (finishedEditingEntry) {
		//Just finished editing a value.
		MenuEntry_t* changedEntry = currentMenu->entries + currentMenu->selectedEntry;
		switch (currentEditMode) {
			case EDIT_NONE: {
				//Check changed entry.
				if (ENTRY_IS("Path")) {
					//Change filepath to load/save.
					strcpy(*filePath, (char*)(changedEntry->data));
				} else if (ENTRY_IS("Save")) {
					//Save
					*(int*)(changedEntry->data) = FALSE;
				} else if (ENTRY_IS("Load")) {
					//Load
					*(int*)(changedEntry->data) = FALSE;
				} else if (ENTRY_IS("Clear")) {
					//Clear
					*(int*)(changedEntry->data) = FALSE;
				}
				break;
			}

			case EDIT_VERTICES: {
				//Check changed entry.
				if (ENTRY_IS("Vertex ID")) {
					unsigned int newID = *(unsigned int*)(changedEntry->data);
					if (newID >= g_numVertices) {break;}

					Vec2f_t* thisVertex = g_vertices + newID;
					mn_updateFloatEntry(currentMenu, "X", thisVertex->x);
					mn_updateFloatEntry(currentMenu, "Y", thisVertex->y);
				} else if (ENTRY_IS("X")) {
					unsigned int vID;
					mn_getUInt(currentMenu, "Vertex ID", &vID);
					Vec2f_t* thisVertex = g_vertices + vID;
					thisVertex->x = *(float*)(changedEntry->data);
				} else if (ENTRY_IS("Y")) {
					unsigned int vID;
					mn_getUInt(currentMenu, "Vertex ID", &vID);
					Vec2f_t* thisVertex = g_vertices + vID;
					thisVertex->y = *(float*)(changedEntry->data);
				}
				break;
			}

			default: {break;}
		}
	}
}
//////// GEOMETRY EDITING ////////

