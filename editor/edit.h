/* edit.h */
#ifndef EDIT_H
#define EDIT_H

//////// CAMERA CTRL ////////
#define MOVEMENT_SPEED_BASE 1.0f /* In units/second */
#define ZOOM_SPEED 2.5f

void io_handleViewportMovement(double dt);
//////// CAMERA CTRL ////////



//////// GEOMETRY EDITING ////////
#define EDITOR_MAX_VERTICES 1024
#define EDITOR_MAX_LINEDEFS 512
#define EDITOR_MAX_SECTORS 128
typedef enum {
	EDIT_NONE, EDIT_VERTICES, EDIT_LINEDEFS, EDIT_SECTORS,
	NUM_EDIT_MODES
} EditMode_e;
extern EditMode_e currentEditMode;

void ed_init(char** filePath);
int ed_update(const Vec2i_t resolutionChars);
//////// GEOMETRY EDITING ////////


#endif