/* physics.h */
#ifndef PHYSICS_H
#define PHYSICS_H

#include "types.h"


//////// DATA ////////
extern unsigned int currentSectorID;
extern int isInPortal;
extern int prevJump;
extern int isOnFloor;
//////// DATA ////////


//////// TICK ////////
void p_updateCamera(Camera_t* camera, double dt);
//////// TICK ////////



#endif