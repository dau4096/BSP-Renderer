/* physics.c */


#include "types.h"
#include "maths.h"
#include "io.h"
#include "graphics.h"



//////// CONSTANTS ////////
#define CAMERA_FEET_OFFSET -1.65f /* Z offset of cameraZ to bottom of camera (bottom of "feet") */
#define CAMERA_HEAD_OFFSET  0.10f /* Z offset of cameraZ to top of camera (top of "head") */

#define GRAVITY -0.01f /* Gravity acceleration in units/second² */
//////// CONSTANTS ////////


Sector_t* p_getSectorID(const Camera_t* camera) {
	//Find the current sector the camera is inside of.
	return sectors; //RET first sector, Actual logic TBA.
}


int isOnFloor = FALSE;
void p_updateCamera(Camera_t* camera, double dt) {
	Vec2f_t movementDelta = io_handleInputs(camera, dt);

	Vec2f_t currentPosition = camera->position;
	Vec2f_t newPosition = v2f_add(currentPosition, movementDelta);


	Sector_t* thisSector = p_getSectorID(camera); //Get the sector that the camera is within.


	//Check if the move intersects any solid linedefs.
	//If it does, then restrict the move.
	camera->position = newPosition; //TBA.



	//Check Z against ceiling/floor of the sector.
	float cameraMinZ = camera->Z + CAMERA_FEET_OFFSET;
	float cameraMaxZ = camera->Z + CAMERA_HEAD_OFFSET;

	camera->Z += camera->Zvelocity;
	if (cameraMaxZ > thisSector->ceilingHeight) {
		//Reset vertical vel, move down.
		camera->Z = thisSector->ceilingHeight - CAMERA_HEAD_OFFSET;
		camera->Zvelocity = 0.0f;

	}
	if ((cameraMinZ < thisSector->floorHeight) && (!isOnFloor)) {
		//Reset vertical vel, move up.
		isOnFloor = TRUE;
		camera->Z = thisSector->floorHeight - CAMERA_FEET_OFFSET;
		camera->Zvelocity = 0.0f;
	} else {
		//Is not in the roof or floor, apply gravity.
		isOnFloor = FALSE;
		camera->Zvelocity += GRAVITY * dt;
	}
}