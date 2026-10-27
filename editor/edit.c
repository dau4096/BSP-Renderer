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
char** filePathPtr;

Menu_t editMenus[NUM_EDIT_MODES];



int ed_saveCBK(void) {
	return f_saveGeo(*filePathPtr);
}

int ed_loadCBK(void) {
	return f_loadGeo(*filePathPtr);
}

int ed_clrCBK(void) {
	//Deallocate if already filled.
	if (g_vertices) {free(g_vertices);}
	if (g_lineDefs) {free(g_lineDefs);}
	if (g_sectors) {free(g_sectors);}

	g_vertices = calloc(EDITOR_MAX_VERTICES, sizeof(Vec2f_t));		g_numVertices = 0u;
	g_lineDefs = calloc(EDITOR_MAX_VERTICES, sizeof(LineDef_t));	g_numLineDefs = 0u;
	g_sectors  = calloc(EDITOR_MAX_VERTICES, sizeof(Sector_t));		g_numSectors  = 0u;

	return TRUE;
}



void ed_updateToVertexID(Menu_t* currentMenu, const unsigned int ID) {
	//Update to new vertex.
	Vec2f_t* thisVertex = g_vertices + ID;
	mn_updateFloatEntry(currentMenu, "X", thisVertex->x);
	mn_updateFloatEntry(currentMenu, "Y", thisVertex->y);
}


void ed_updateToLineDefID(Menu_t* currentMenu, const unsigned int ID) {
	//Update to new linedef.
	LineDef_t* thisLineDef = g_lineDefs + ID;
	mn_updateUIntEntry(currentMenu, "Start vID", thisLineDef->vStart);
	mn_updateUIntEntry(currentMenu, "Start vID", thisLineDef->vStart);
	mn_updateIntEntry(currentMenu, "Front sID", thisLineDef->frontSector);
	mn_updateIntEntry(currentMenu, "Back sID", thisLineDef->backSector);
	mn_updateStringEntry(currentMenu, "Texture", (char*)(textureNames[thisLineDef->texture]));
}


void ed_updateToSectorID(Menu_t* currentMenu, const unsigned int ID) {
	//Update to new sector.
	Sector_t* thisSector = g_sectors + ID;
	mn_updateUIntEntry(currentMenu, "Light Level", thisSector->lightLevel);
	
	int isFloorTextured = thisSector->flags & 0x1u > 0u;
	mn_updateFloatEntry(currentMenu, "[F] Height", thisSector->floorHeight);
	mn_updateBoolEntry(currentMenu, "[F] Textured", isFloorTextured);
	if (isFloorTextured) {
		//Update texture listing
		mn_updateStringEntry(currentMenu, "[F] Texture", (char*)(textureNames[thisSector->floorTexture]));
	} else {
		//Update R/G/B
		mn_updateUIntEntry(currentMenu, "[F] R", thisSector->floorColour.r);
		mn_updateUIntEntry(currentMenu, "[F] G", thisSector->floorColour.g);
		mn_updateUIntEntry(currentMenu, "[F] B", thisSector->floorColour.b);
	}
	
	int isCeilingTextured = thisSector->flags & 0x2u > 0u;
	mn_updateFloatEntry(currentMenu, "[C] Height", thisSector->ceilingHeight);
	mn_updateBoolEntry(currentMenu, "[C] Textured", isCeilingTextured);
	if (isCeilingTextured) {
		//Update texture listing
		mn_updateStringEntry(currentMenu, "[F] Texture", (char*)(textureNames[thisSector->ceilingTexture]));
	} else {
		//Update R/G/B
		mn_updateUIntEntry(currentMenu, "[C] R", thisSector->ceilingColour.r);
		mn_updateUIntEntry(currentMenu, "[C] G", thisSector->ceilingColour.g);
		mn_updateUIntEntry(currentMenu, "[C] B", thisSector->ceilingColour.b);
	}

	mn_updateUIntEntry(currentMenu, "#LineDefs", thisSector->numLineDefs);
	char title[16u];
	for (unsigned int i=0u; i<thisSector->numLineDefs; i++) {
		sprintf(title, "LD[%d]", i);
		mn_updateUIntEntry(currentMenu, title, thisSector->lineDefs[i]);
	}
}




int ed_addNewVertexCBK(void) {
	if (g_numVertices >= EDITOR_MAX_VERTICES) {return FALSE;}
	g_vertices[g_numVertices] = (Vec2f_t){
		.x=0.0f, .y=0.0f
	};

	Menu_t* currentMenu = editMenus + currentEditMode;
	mn_updateUIntEntry(currentMenu, "Vertex ID", g_numVertices);
	ed_updateToVertexID(currentMenu, g_numVertices++);
	return TRUE;
}

int ed_addNewLineDefCBK(void) {
	if (g_numLineDefs >= EDITOR_MAX_LINEDEFS) {return FALSE;}
	g_lineDefs[g_numLineDefs] = (LineDef_t){
		.vStart=0, .vEnd=0,
		.frontSector=-1, .backSector=-1,
		.texture=0u,
		.isValid=TRUE
	};

	Menu_t* currentMenu = editMenus + currentEditMode;
	mn_updateUIntEntry(currentMenu, "LineDef ID", g_numLineDefs);
	ed_updateToLineDefID(currentMenu, g_numLineDefs++);
	return TRUE;
}

int ed_addNewSectorCBK(void) {
	if (g_numSectors >= EDITOR_MAX_SECTORS) {return FALSE;}
	g_sectors[g_numSectors] = (Sector_t){
		.floorHeight=0.0f, .floorTexture=0u, .floorColour=RGB_BLACK,
		.ceilingHeight=0.0f, .ceilingTexture=0u, .ceilingColour=RGB_BLACK,
		.flags=0b0000'0000, .lineDefs=NULL, .numLineDefs=0u,
		.lightLevel=0xFF, .isValid = TRUE
	};

	Menu_t* currentMenu = editMenus + currentEditMode;
	mn_updateUIntEntry(currentMenu, "Sector ID", g_numSectors);
	ed_updateToSectorID(currentMenu, g_numSectors++);
	return TRUE;
}






int ed_deleteVertexCBK(void) {
	if (g_numVertices == 0u) {return FALSE;}
	Menu_t* currentMenu = editMenus + currentEditMode;
	unsigned int ID;
	mn_getUInt(currentMenu, "Vertex ID", &ID);
	g_vertices[ID].x = 0.0f;
	g_vertices[ID].y = 0.0f;
	if (ID > 0u) {ed_updateToVertexID(currentMenu, ID-1);}
	else {ed_updateToVertexID(currentMenu, ID+1);}
	return TRUE;
}

int ed_deleteLineDefCBK(void) {
	if (g_numLineDefs == 0u) {return FALSE;}
	Menu_t* currentMenu = editMenus + currentEditMode;
	unsigned int ID;
	mn_getUInt(currentMenu, "LineDef ID", &ID);
	g_lineDefs[ID].isValid = FALSE;
	if (ID > 0u) {ed_updateToLineDefID(currentMenu, ID-1);}
	else {ed_updateToLineDefID(currentMenu, ID+1);}
	return TRUE;
}

int ed_deleteSectorCBK(void) {
	if (g_numSectors == 0u) {return FALSE;}
	Menu_t* currentMenu = editMenus + currentEditMode;
	unsigned int ID;
	mn_getUInt(currentMenu, "Sector ID", &ID);
	g_sectors[ID].isValid = FALSE;
	if (ID > 0u) {ed_updateToSectorID(currentMenu, ID-1);}
	else {ed_updateToSectorID(currentMenu, ID+1);}
	return TRUE;
}




void ed_init(char** fpPtr) {
	filePathPtr = fpPtr;

	//Create all of the vertices controls
	Menu_t* saveloadMenu = editMenus + EDIT_NONE;
	mn_createNewMenu("Save/Load", saveloadMenu);
	mn_addSeperator(saveloadMenu, "");
	mn_addStringEntry(saveloadMenu, "Path", *filePathPtr);
	mn_addCallbackEntry(saveloadMenu, "Save", ed_saveCBK);
	mn_addCallbackEntry(saveloadMenu, "Load", ed_loadCBK);
	mn_addCallbackEntry(saveloadMenu, "Clear", ed_clrCBK);




	Menu_t* verticesMenu = editMenus + EDIT_VERTICES;
	mn_createNewMenu("Vertices", verticesMenu);
	mn_addSeperator(verticesMenu, "");
	mn_addUIntEntry(verticesMenu, "Vertex ID", 0u);
	mn_addCallbackEntry(verticesMenu, "Add", ed_addNewVertexCBK);
	mn_addCallbackEntry(verticesMenu, "Delete", ed_deleteVertexCBK);
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
	mn_addCallbackEntry(linedefsMenu, "Add", ed_addNewLineDefCBK);
	mn_addCallbackEntry(linedefsMenu, "Delete", ed_deleteLineDefCBK);
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
	mn_addCallbackEntry(sectorsMenu, "Add", ed_addNewSectorCBK);
	mn_addCallbackEntry(sectorsMenu, "Delete", ed_deleteSectorCBK);
	mn_addSeperator(sectorsMenu, "Geometry");
	if (g_numSectors > 0u) {
		mn_addUIntEntry(sectorsMenu, "Light Level", g_sectors->lightLevel);
		
		mn_addSeperator(sectorsMenu, "Floor");
		mn_addFloatEntry(sectorsMenu, "[F] Height", g_sectors->floorHeight);
		int isFloorTextured = g_sectors->flags & 0x1u;
		mn_addBoolEntry(sectorsMenu, "[F] Textured", isFloorTextured);
		if (isFloorTextured > 0u)  {
			mn_addStringEntry(sectorsMenu, "[F] Texture", textureNames[g_sectors->floorTexture]);
		} else {
			mn_addUIntEntry(sectorsMenu, "[F] R", g_sectors->floorColour.r);
			mn_addUIntEntry(sectorsMenu, "[F] G", g_sectors->floorColour.g);
			mn_addUIntEntry(sectorsMenu, "[F] B", g_sectors->floorColour.b);
		}

		mn_addSeperator(sectorsMenu, "Ceiling");
		mn_addFloatEntry(sectorsMenu, "[C] Height", g_sectors->ceilingHeight);
		int isCeilingTextured = g_sectors->flags & 0x2u;
		mn_addBoolEntry(sectorsMenu, "[C] Textured", isCeilingTextured);
		if (isCeilingTextured > 0u)  {
			mn_addStringEntry(sectorsMenu, "[C] Texture", textureNames[g_sectors->ceilingTexture]);
		} else {
			mn_addUIntEntry(sectorsMenu, "[C] R", g_sectors->ceilingColour.r);
			mn_addUIntEntry(sectorsMenu, "[C] G", g_sectors->ceilingColour.g);
			mn_addUIntEntry(sectorsMenu, "[C] B", g_sectors->ceilingColour.b);			
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
		mn_addFloatEntry(sectorsMenu, "[F] Height", 0.0f);
		mn_addBoolEntry(sectorsMenu, "[F] Use Texture", TRUE);
		mn_addStringEntry(sectorsMenu, "[F] Texture", "textures/fallback.png");

		mn_addSeperator(sectorsMenu, "Ceiling");
		mn_addFloatEntry(sectorsMenu, "[C] Height", 0.0f);
		mn_addBoolEntry(sectorsMenu, "[C] Use Texture", TRUE);
		mn_addStringEntry(sectorsMenu, "[C] Texture", "textures/fallback.png");

		mn_addUIntEntry(sectorsMenu, "#LineDefs", 0u);
		mn_addSeperator(sectorsMenu, "LineDefs");
	}
}


#define EQU(X, Y) (strcmp((X), (Y)) == 0u)
#define ENTRY_IS(Y) (EQU(changedEntry->title, (Y)))
int ed_update(const Vec2i_t resolutionChars) {
	if (!isEditingEntry && keyMapPress[K_TURN_LEFT])  {
		if (currentEditMode > 0) {currentEditMode--;}
		else {currentEditMode = NUM_EDIT_MODES;}
	}
	if (!isEditingEntry && keyMapPress[K_TURN_RIGHT]) {currentEditMode = (currentEditMode+1) % NUM_EDIT_MODES;}

	Menu_t* currentMenu = editMenus + currentEditMode;
	mn_editEntry(currentMenu);
	if (!isEditingEntry && keyMapPress[K_NAV_UP]) {
		if (currentMenu->selectedEntry > 0) {currentMenu->selectedEntry--;}
		else {currentMenu->selectedEntry = currentMenu->numEntries - 1u;}
	}
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
					strcpy(*filePathPtr, (char*)(changedEntry->data));
				}
				break;
			}

			case EDIT_VERTICES: {
				//Check changed entry.
				if (ENTRY_IS("Vertex ID")) {
					unsigned int newID = *(unsigned int*)(changedEntry->data);
					if (newID >= g_numVertices) {
						newID = g_numVertices-1u;
						mn_updateUIntEntry(currentMenu, "Vertex ID", newID);
					}
					ed_updateToVertexID(currentMenu, newID);
				
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

			case EDIT_LINEDEFS: {
				//Check changed entry.
				if (ENTRY_IS("LineDef ID")) {
					unsigned int newID = *(unsigned int*)(changedEntry->data);
					if ((newID >= g_numLineDefs) || (!g_lineDefs[newID].isValid)) {
						newID = g_numLineDefs-1u;
						mn_updateUIntEntry(currentMenu, "LineDef ID", newID);
					}
					ed_updateToLineDefID(currentMenu, newID);
				
				} else if (ENTRY_IS("Start vID")) {
					unsigned int ldID;
					mn_getUInt(currentMenu, "LineDef ID", &ldID);
					LineDef_t* thisLineDef = g_lineDefs + ldID;

					unsigned int newID = *(unsigned int*)(changedEntry->data);
					if (newID >= g_numVertices) {
						mn_updateUIntEntry(currentMenu, "Start vID", thisLineDef->vStart);
						break;
					}

					thisLineDef->vStart = newID;

				} else if (ENTRY_IS("End vID")) {
					unsigned int ldID;
					mn_getUInt(currentMenu, "LineDef ID", &ldID);
					LineDef_t* thisLineDef = g_lineDefs + ldID;

					unsigned int newID = *(unsigned int*)(changedEntry->data);
					if (newID >= g_numVertices) {
						mn_updateUIntEntry(currentMenu, "End vID", thisLineDef->vEnd);
						break;
					}

					thisLineDef->vEnd = newID;

				} else if (ENTRY_IS("Front sID")) {
					unsigned int ldID;
					mn_getUInt(currentMenu, "LineDef ID", &ldID);
					LineDef_t* thisLineDef = g_lineDefs + ldID;

					unsigned int newID = *(unsigned int*)(changedEntry->data);
					if (newID >= g_numSectors) {
						mn_updateUIntEntry(currentMenu, "Front sID", thisLineDef->frontSector);
						break;
					}

					thisLineDef->frontSector = newID;

				} else if (ENTRY_IS("Back sID")) {
					unsigned int ldID;
					mn_getUInt(currentMenu, "LineDef ID", &ldID);
					LineDef_t* thisLineDef = g_lineDefs + ldID;

					unsigned int newID = *(unsigned int*)(changedEntry->data);
					if (newID >= g_numSectors) {
						mn_updateUIntEntry(currentMenu, "Back sID", thisLineDef->backSector);
						break;
					}

					thisLineDef->backSector = newID;

				} else if (ENTRY_IS("Texture")) {
					char* newPath = (char*)(changedEntry->data);
					unsigned int ldID;
					mn_getUInt(currentMenu, "LineDef ID", &ldID);
					LineDef_t* thisLineDef = g_lineDefs + ldID;

					//Do something, eventually. Maybe assigning some ID?
					//ed_assignNewTextureID();
					printf("\n<NOT-IMPL> LD Texture Change [%s}→{%s]\n", (char*)(textureNames[thisLineDef->texture]), newPath);
					return FALSE;
				}
			}

			case EDIT_SECTORS: {
				//Check changed entry.
				if (ENTRY_IS("Sector ID")) {
					unsigned int newID = *(unsigned int*)(changedEntry->data);
					if ((newID >= g_numSectors) || (!g_sectors[newID].isValid)) {
						newID = g_numSectors-1u;
						mn_updateUIntEntry(currentMenu, "Sector ID", newID);
					}
					ed_updateToSectorID(currentMenu, newID);

				} else if (ENTRY_IS("Light Level")) {
					unsigned int sID;
					mn_getUInt(currentMenu, "Sector ID", &sID);
					Sector_t* thisSector = g_sectors + sID;

					unsigned int newLL = *(unsigned int*)(changedEntry->data);
					if (newLL >= 0xFF) {
						mn_updateUIntEntry(currentMenu, "Light Level", 0xFF);
						break;
					}

					thisSector->lightLevel = MIN(newLL, 0xFF);
				
				} else if (ENTRY_IS("[F] Height")) {
					unsigned int sID;
					mn_getUInt(currentMenu, "Sector ID", &sID);
					Sector_t* thisSector = g_sectors + sID;
					unsigned int newZ = *(unsigned int*)(changedEntry->data);
					thisSector->floorHeight = newZ;

				} else if (ENTRY_IS("[F] Textured")) {
					unsigned int sID;
					mn_getUInt(currentMenu, "Sector ID", &sID);
					Sector_t* thisSector = g_sectors + sID;
					unsigned int newZ = *(unsigned int*)(changedEntry->data);

					int isFloorTextured = *(int*)(changedEntry->data) != FALSE;
					thisSector->flags &= 0xFFFFFFFEu;
					thisSector->flags |= isFloorTextured & 0x1u;

					printf("\n<NOT-IMPL> Sector Texture status change unfinished.\n");
					return FALSE;

					if (isFloorTextured) {
						mn_deleteEntry(currentMenu, "[F] R");
						mn_deleteEntry(currentMenu, "[F] G");
						mn_deleteEntry(currentMenu, "[F] B");
					} else {
						mn_deleteEntry(currentMenu, "[F] Texture");
					}

				} else if (ENTRY_IS("[F] Texture")) {
					char* newPath = (char*)(changedEntry->data);
					unsigned int sID;
					mn_getUInt(currentMenu, "LineDef ID", &sID);
					Sector_t* thisSector = g_sectors + sID;

					//Do something, eventually. Maybe assigning some ID?
					//ed_assignNewTextureID();
					printf("\n<NOT-IMPL> SEC Texture Change [%s}→{%s]\n", (char*)(textureNames[thisSector->floorTexture]), newPath);
					return FALSE;

				} else if (ENTRY_IS("[F] R")) {
					unsigned int sID;
					mn_getUInt(currentMenu, "Sector ID", &sID);
					Sector_t* thisSector = g_sectors + sID;

					unsigned int newC = *(unsigned int*)(changedEntry->data);
					if (newC >= 0xFF) {
						mn_updateUIntEntry(currentMenu, "[F] R", 0xFF);
						break;
					}

					thisSector->floorColour.r = MIN(newC, 0xFF);

				} else if (ENTRY_IS("[F] G")) {
					unsigned int sID;
					mn_getUInt(currentMenu, "Sector ID", &sID);
					Sector_t* thisSector = g_sectors + sID;

					unsigned int newC = *(unsigned int*)(changedEntry->data);
					if (newC >= 0xFF) {
						mn_updateUIntEntry(currentMenu, "[F] G", 0xFF);
						break;
					}

					thisSector->floorColour.g = MIN(newC, 0xFF);

				} else if (ENTRY_IS("[F] B")) {
					unsigned int sID;
					mn_getUInt(currentMenu, "Sector ID", &sID);
					Sector_t* thisSector = g_sectors + sID;

					unsigned int newC = *(unsigned int*)(changedEntry->data);
					if (newC >= 0xFF) {
						mn_updateUIntEntry(currentMenu, "[F] B", 0xFF);
						break;
					}

					thisSector->floorColour.b = MIN(newC, 0xFF);

				} else if (ENTRY_IS("[C] Height")) {
					unsigned int sID;
					mn_getUInt(currentMenu, "Sector ID", &sID);
					Sector_t* thisSector = g_sectors + sID;
					unsigned int newZ = *(unsigned int*)(changedEntry->data);
					thisSector->ceilingHeight = newZ;

				} else if (ENTRY_IS("[C] Textured")) {
					unsigned int sID;
					mn_getUInt(currentMenu, "Sector ID", &sID);
					Sector_t* thisSector = g_sectors + sID;
					unsigned int newZ = *(unsigned int*)(changedEntry->data);

					int isCeilingTextured = *(int*)(changedEntry->data) != FALSE;
					thisSector->flags &= 0xFFFFFFFDu;
					thisSector->flags |= (isCeilingTextured & 0x2u) << 1u;

					printf("\n<NOT-IMPL> Sector Texture status change unfinished.\n");
					return FALSE;

					if (isCeilingTextured) {
						mn_deleteEntry(currentMenu, "[C] R");
						mn_deleteEntry(currentMenu, "[C] G");
						mn_deleteEntry(currentMenu, "[C] B");
					} else {
						mn_deleteEntry(currentMenu, "[C] Texture");
					}

				} else if (ENTRY_IS("[C] Texture")) {
					char* newPath = (char*)(changedEntry->data);
					unsigned int sID;
					mn_getUInt(currentMenu, "LineDef ID", &sID);
					Sector_t* thisSector = g_sectors + sID;

					//Do something, eventually. Maybe assigning some ID?
					//ed_assignNewTextureID();
					printf("\n<NOT-IMPL> SEC Texture Change [%s}→{%s]\n", (char*)(textureNames[thisSector->ceilingTexture]), newPath);
					return FALSE;

				} else if (ENTRY_IS("[C] R")) {
					unsigned int sID;
					mn_getUInt(currentMenu, "Sector ID", &sID);
					Sector_t* thisSector = g_sectors + sID;

					unsigned int newC = *(unsigned int*)(changedEntry->data);
					if (newC >= 0xFF) {
						mn_updateUIntEntry(currentMenu, "[C] R", 0xFF);
						break;
					}

					thisSector->ceilingColour.r = MIN(newC, 0xFF);

				} else if (ENTRY_IS("[C] G")) {
					unsigned int sID;
					mn_getUInt(currentMenu, "Sector ID", &sID);
					Sector_t* thisSector = g_sectors + sID;

					unsigned int newC = *(unsigned int*)(changedEntry->data);
					if (newC >= 0xFF) {
						mn_updateUIntEntry(currentMenu, "[C] G", 0xFF);
						break;
					}

					thisSector->ceilingColour.g = MIN(newC, 0xFF);

				} else if (ENTRY_IS("[C] B")) {
					unsigned int sID;
					mn_getUInt(currentMenu, "Sector ID", &sID);
					Sector_t* thisSector = g_sectors + sID;

					unsigned int newC = *(unsigned int*)(changedEntry->data);
					if (newC >= 0xFF) {
						mn_updateUIntEntry(currentMenu, "[C] B", 0xFF);
						break;
					}

					thisSector->ceilingColour.b = MIN(newC, 0xFF);

				} else if (ENTRY_IS("#LineDefs")) {
					//Change size of LD array..? Going to get complicated.
					unsigned int sID;
					mn_getUInt(currentMenu, "Sector ID", &sID);
					Sector_t* thisSector = g_sectors + sID;

					unsigned int newCount = *(unsigned int*)(changedEntry->data);
					if ((newCount >= g_numLineDefs) || (newCount == 0u)) {
						mn_updateUIntEntry(currentMenu, "#LineDefs", thisSector->numLineDefs);
						break;
					}

					//Temporary buffer.
					unsigned int* temporaryLDbuffer = malloc(newCount * sizeof(unsigned int));
					memcpy(temporaryLDbuffer, thisSector->lineDefs, MIN(newCount, thisSector->numLineDefs) * sizeof(unsigned int));
					free(thisSector->lineDefs);
					thisSector->lineDefs = malloc(newCount * sizeof(unsigned int));
					memcpy(thisSector->lineDefs, temporaryLDbuffer, newCount * sizeof(unsigned int));
					free(temporaryLDbuffer);

					//Remove old LD entries, add new ones.
					char title[16u];
					for (unsigned int i=0u; i<thisSector->numLineDefs; i++) {
						sprintf(title, "LD[%d]", i);
						mn_deleteEntry(currentMenu, title);
					}
					for (unsigned int i=0u; i<thisSector->numLineDefs; i++) {
						sprintf(title, "LD[%d]", i);
						mn_addUIntEntry(currentMenu, title, thisSector->lineDefs[i]);
					}
				}
			}

			default: {break;}
		}
	}

	return TRUE;
}
//////// GEOMETRY EDITING ////////

