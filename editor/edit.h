/* edit.h */
#ifndef EDIT_H
#define EDIT_H

//////// CAMERA CTRL ////////
#define MOVEMENT_SPEED_BASE 1.0f /* In units/second */
#define ZOOM_SPEED 2.5f

void io_handleViewportMovement(double dt);
//////// CAMERA CTRL ////////



//////// GEOMETRY EDITING ////////
typedef enum {
	EDIT_NONE, EDIT_VERTICES, EDIT_LINEDEFS, EDIT_SECTORS,
	NUM_EDIT_MODES
} EditMode_e;
extern EditMode_e currentEditMode;

void ed_init(const char* filePath);
void ed_update(const Vec2i_t resolutionChars, char** filePath);
//////// GEOMETRY EDITING ////////


#endif