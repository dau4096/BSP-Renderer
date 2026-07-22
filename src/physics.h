/* physics.h */
#ifndef PHYSICS_H
#define PHYSICS_H

#include "types.h"


//////// DATA ////////
extern unsigned int currentSectorID;
extern int isInPortal;
extern int isOnFloor;
//////// DATA ////////


//////// TICK ////////
Sector_t* p_findCurrentSectorSlow(Camera_t* camera);

void p_updateCamera(Camera_t* camera, double dt);
//////// TICK ////////



#endif