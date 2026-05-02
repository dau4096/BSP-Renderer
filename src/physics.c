/* physics.c */
#include <stdio.h>


#include "types.h"
#include "maths.h"
#include "io.h"
#include "graphics.h"



//////// CONSTANTS ////////
#define CAMERA_FEET_OFFSET -1.65f /* Z offset of cameraZ to bottom of camera (bottom of "feet") */
#define CAMERA_HEAD_OFFSET  0.10f /* Z offset of cameraZ to top of camera (top of "head") */
#define CAMERA_HIT_RADIUS   0.25f /* Cylindrical collision: radius. */
#define CAMERA_MAX_STEP     0.25f /* Maximum Z-delta the camera is allowed to step up. */

#define MOVEMENT_SPEED_BASE 2.0f /* In units/second */
#define TURNING_SPEED 2.75f/* In radians/second */
#define JUMP_SPEED 0.25f /* 1-time impulse */

#define GRAVITY -0.01f /* Gravity acceleration in units/second² */
//////// CONSTANTS ////////

typedef struct {Vec2f_t start, end, delta;} Movement_t; //Contains info about the camera's movement.


//Check if LD is a portal or a solid wall.
int p_isLineDefSolid(const LineDef_t* thisLineDef) {return (thisLineDef->frontSector == -1) || (thisLineDef->backSector == -1);}


int p_circleLineIntersect(
	const Vec2f_t start, const Vec2f_t end,
	const Vec2f_t circlePosition, const float radius,
	float* distToLine
) {
	Vec2f_t lineDir = v2f_sub(end, start);
	Vec2f_t lineToCircle = v2f_sub(circlePosition, start);

	float t = v2f_dot(lineToCircle, lineDir) / v2f_lenSQ(lineDir);
	t = CLAMP(t, 0.0f, 1.0f);

	Vec2f_t closestPoint = v2f_add(start, v2f_mul(lineDir, t));
	float distToCircle = v2f_dist(circlePosition, closestPoint);

	if (distToLine) {
		*distToLine = distToCircle;
	}
	return distToCircle <= radius;
}


int p_getLDintersect(LineDef_t* thisLineDef, const Movement_t* motion, float* intersectDistance) {
	//Get the intersect between a LD and some movement.
	//Camera is modelled as a cylinder, so anything within (radius) is an intersect.
	Vec2f_t start = g_vertices[thisLineDef->vStart];
	Vec2f_t end = g_vertices[thisLineDef->vEnd];

	//Check the ends;
	float dMeLDs = v2f_dist(motion->end,   start);
	float dMsLDs = v2f_dist(motion->start, start);
	float dMsLDe = v2f_dist(motion->start, end  );
	float dMeLDe = v2f_dist(motion->end,   end  );
	float minDist = MIN(MIN(dMsLDs, dMeLDs), MIN(dMeLDs, dMeLDe));
	if (minDist <= CAMERA_HIT_RADIUS) {
		//Intersects at one of the ends.
		return TRUE;
	}

	
	//Check the actual line;
	if (p_circleLineIntersect(start, end, motion->start, CAMERA_HIT_RADIUS, intersectDistance)) {
		//Must have hit somewhere along the line.
		return TRUE;
	}

	return FALSE;
}


#define currentPosition camera->position
unsigned int currentSectorID = 0u; //Init at 0, maybe add some checking later for where to start.
int p_getSectorID(
	const Camera_t* camera, Sector_t** currentSector, Movement_t* motion
) { //Returns whether or not a linedef was hit.
	//Find the current sector the camera is inside of.
	//Stores state between frames, only updates when crossing a linedef.
	Sector_t* thisSector = g_sectors + currentSectorID;

	//Check each linedef to see if it crossed it. If so then either change sector or limit travel dist.
	int hitLineDef = FALSE;
	for (unsigned int ldIndex=0u; ldIndex<thisSector->numLineDefs; ldIndex++) {
		LineDef_t* thisLineDef = g_lineDefs + thisSector->lineDefs[ldIndex];
		float intersectDistance;
		int intersected = p_getLDintersect(thisLineDef, motion, &intersectDistance);
		if (!intersected) {continue; /* Move onto the next LD. */}

		hitLineDef = TRUE;
		int isSolid = p_isLineDefSolid(thisLineDef);
		if (!isSolid) {
			//Portal; check camera can "step" up the Z-delta before allowing through. Otherwise restrict.
			//If can step through, change ID to be new sector

			unsigned int newSectorID; //ID of new sector to check.
			if (thisLineDef->frontSector == currentSectorID) {newSectorID = thisLineDef->backSector;}
			else {newSectorID = thisLineDef->frontSector;}
			Sector_t* newSector = g_sectors + newSectorID; //New sector to check against.

			float cameraFootZ = camera->Z + CAMERA_FEET_OFFSET;
			if (newSector->floorHeight > (cameraFootZ + CAMERA_MAX_STEP)) {isSolid = TRUE; /* Can't step up this, too high. */}
			else {
				//Can walk through, change to new sector.
				currentSectorID = newSectorID;
				thisSector = newSector;
			}
		}

		if (isSolid) {
			//Solid wall, restrict motion so camera doesn't pass through.
			//Can also happen from portals with too high step delta.
			Vec2f_t offset = v2f_mul(
				v2f_normalise(motion->delta),
				-intersectDistance
			);
			motion->end = v2f_add(motion->end, offset);
			motion->delta = v2f_sub(motion->end, motion->start);
		}

		break; //No need to check any more.
	}
	*currentSector = thisSector;
	if (!hitLineDef) {return FALSE;}


	return TRUE;
}





int prevJump = FALSE;
int isOnFloor = FALSE;
Vec2f_t p_handleInputs(Camera_t* camera, double dt) {
	//Camera Controls
	if (keyMap[K_TURN_LEFT]) {camera->yaw -= TURNING_SPEED * dt;}
	if (keyMap[K_TURN_RIGHT]) {camera->yaw += TURNING_SPEED * dt;}
	camera->yaw = fmod(camera->yaw, 2.0f * M_PI);
	if (camera->yaw < 0.0f) {camera->yaw += 2.0f * M_PI;}

	camera->forward = (Vec2f_t){
		.x=sin(camera->yaw), .y=cos(camera->yaw)
	};


	//Move camera based on inputs.
	float movementSpeed = MOVEMENT_SPEED_BASE * dt;
	if (keyMap[K_MOVE_FAST]) {movementSpeed *= 2.5f;}
	Vec2f_t forward = v2f_mul(camera->forward, movementSpeed);
	Vec2f_t right = (Vec2f_t){.x=forward.y, .y=-forward.x};

	Vec2f_t moveDelta = (Vec2f_t){.x=0.0f, .y=0.0f};
	if (keyMap[K_MOVE_FORE]) {moveDelta = v2f_add(moveDelta, forward);}
	if (keyMap[K_MOVE_BACK]) {moveDelta = v2f_sub(moveDelta, forward);}
	if (keyMap[K_MOVE_LEFT]) {moveDelta = v2f_sub(moveDelta, right);}
	if (keyMap[K_MOVE_RIGHT]) {moveDelta = v2f_add(moveDelta, right);}

	if (keyMap[K_MOVE_JUMP] && !prevJump && isOnFloor) {camera->Zvelocity += JUMP_SPEED;}
	prevJump = keyMap[K_MOVE_JUMP];

	return moveDelta;
}




void p_updateCamera(Camera_t* camera, double dt) {
	Vec2f_t movementDelta = p_handleInputs(camera, dt);


	Movement_t motion = (Movement_t){
		.start=camera->position,
		.end=v2f_add(camera->position, movementDelta)
	};
	motion.delta=v2f_sub(motion.end, motion.start);


	LineDef_t* intersectedLinedef; //Only assigned if one was hit.
	Sector_t* thisSector;
	int hitLineDef = p_getSectorID(
		camera,	&thisSector, &motion
	); //Get the sector that the camera is within.
	camera->position = motion.end;

#ifdef SUPPRESS_FRAMEBUFFER_OUTPUT
	printf("Start: (%f, %f), End: (%f, %f)\n", motion.start.x, motion.start.y, motion.end.x, motion.end.y);
#endif

	//Check Z against ceiling/floor of the sector.
	camera->Z += camera->Zvelocity;
	float cameraFootZ = camera->Z + CAMERA_FEET_OFFSET;
	float cameraHeadZ = camera->Z + CAMERA_HEAD_OFFSET;

	if (cameraHeadZ > thisSector->ceilingHeight) {
		//Reset vertical vel, move down.
		camera->Z = thisSector->ceilingHeight - CAMERA_HEAD_OFFSET;
		camera->Zvelocity = 0.0f;
	}

	if ((cameraFootZ < thisSector->floorHeight) && (!isOnFloor)) {
		//Reset vertical vel, move up.
		isOnFloor = TRUE;
		camera->Z = thisSector->floorHeight - CAMERA_FEET_OFFSET;
		camera->Zvelocity = 0.0f;
	} else if (cameraFootZ >= thisSector->floorHeight) {
		//Is not in the roof or floor, apply gravity.
		isOnFloor = FALSE;
		camera->Zvelocity += GRAVITY * dt;
	}
}

