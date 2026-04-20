/* graphics.c */

#include <stdlib.h>
#include <stdio.h>

#include "types.h"
#include "maths.h"
#include "terminal.h"


//Temporary, while camera is in 2D.
#define cameraZ 0.0f
Camera_t camera; //The view


//Placeholder values; replace with calloc() heap stuff later.
#define MAX_VERTICES 64
#define MAX_LINEDEFS 32
#define MAX_SECTORS  16

Vec2f_t vertices[MAX_VERTICES]; //2D positions.
LineDef_t lineDefs[MAX_LINEDEFS]; //LineDefs connecting vertices.
Sector_t sectors[MAX_SECTORS]; //Sectors made of LineDefs.




int r_getCentreX(const Vec2f_t position, const Vec2i_t resolution) {
	Vec2f_t direction = v2f_sub(position, camera.position);

	float theta = atan2(direction.x, direction.y);
	float angleDelta = theta - camera.yaw;

	if (angleDelta >  180.0f) {angleDelta -= 360.0f;}
	if (angleDelta < -180.0f) {angleDelta += 360.0f;}

	float centreX = ((float)(resolution.x) / 2.0f) * ((angleDelta * 2.0f / camera.FOV) + 1.0f);
	return (int)(centreX);
}


void r_getLineDefSectorProjections(
	const int sectorID, float invDistance, int* lowYBound, int* topYBound, const Vec2i_t resolution
) {
	const Sector_t* thisSector = sectors + sectorID;
	float projectedYFloor = (cameraZ - thisSector->floorHeight) * invDistance;
	float projectedYCeiling = (cameraZ - thisSector->ceilingHeight) * invDistance;

	*lowYBound = (int)((float)(resolution.y) * (0.5f - projectedYFloor));
	*topYBound = (int)((float)(resolution.y) * (0.5f - projectedYCeiling));
}


static int tmp = FALSE;
void r_drawColumn(const int sectorID, int x, float invDistance, RGB_t* fbPTR, const Vec2i_t resolution, RGB_t colour) {
	//Draw this wall collumn.
	int lowYBound, topYBound;
	r_getLineDefSectorProjections(
		sectorID, invDistance, &lowYBound, &topYBound, resolution
	);
	if ((topYBound<0) || (lowYBound>=resolution.y)) {return; /* Completely offscreen vertically. */}

	//printf("Found LowBound: %d, HighBound: %d\n", lowYBound, topYBound);
	int yLow = fmax(lowYBound, 0);
	int yTop = fmin(topYBound, resolution.y-1);
	//printf("Found Low: %d, High: %d\n", yLow, yTop);


	//Draw.
	RGB_t* ptr = fbPTR + x + (resolution.x * yLow);
	for (int y=yLow; y<yTop; y++) {
		//Texturing TBA. yLow is low INDEX not low ONSCREEN.
		*ptr = colour; //Magenta for now.
		ptr += resolution.x;
	}
}



void r_drawLineDef(const LineDef_t* thisLineDef, const Vec2i_t resolution, RGB_t* fbPTR) {
	//Interpolate from start-end along the LineDef.
	Vec2f_t start = vertices[thisLineDef->vStart];
	Vec2f_t end = vertices[thisLineDef->vEnd];

	//Project into screen horizontally
	int startX = r_getCentreX(start, resolution);
	int endX = r_getCentreX(end, resolution);
	//printf("Start: %d, End: %d\n", startX, endX);
	if (startX == endX) {return; /* Infinitely thin, don't draw. */}

	//Calculate depth
	float startDepth = v2f_len(v2f_sub(start, camera.position));
	float endDepth = v2f_len(v2f_sub(end, camera.position));

	//Find left/rightmost
	int leftMost, rightMost;
	float lInvDepth, rInvDepth;
	if (startX < endX) {
		leftMost = startX; lInvDepth = 1.0f / startDepth;
		rightMost = endX;  rInvDepth = 1.0f / endDepth;
	} else {
		rightMost = startX; rInvDepth = 1.0f / startDepth;
		leftMost = endX;    lInvDepth = 1.0f / endDepth;
	}
	int range = rightMost - leftMost;

	leftMost = fmax(leftMost, 0);
	rightMost = fmin(rightMost, resolution.x);


	//Draw, interpolating.
	float aspectRatio = (float)(resolution.x) / (float)(resolution.y);
	printf("L: %d, R: %d\n", leftMost, rightMost);
	for (int x=leftMost; x<rightMost; x++) {
		float t = (float)(x - leftMost) / (float)(range);
		float invDistance = aspectRatio * f_lerp(lInvDepth, rInvDepth, t);
		r_drawColumn(thisLineDef->frontSector, x, invDistance, fbPTR, resolution, thisLineDef->colour);
	}
}


void r_drawSector(const Sector_t* thisSector, const Vec2i_t resolution, RGB_t* fbPTR) {
	//Loop through it's linedefs, drawing those.
	for (unsigned int secLD=0u; secLD<thisSector->numLineDefs; secLD++) {
		unsigned int ldIndex = thisSector->lineDefs[secLD];
		const LineDef_t* thisLineDef = lineDefs + ldIndex;
		r_drawLineDef(thisLineDef, resolution, fbPTR);
	}
}



void r_drawFrame(const Vec2i_t resolution) {
	//TBA
	RGB_t* fbPTR = t_getFramebufferPTR();

	tmp = FALSE;
	for (unsigned int sIndex=0u; sIndex<MAX_SECTORS; sIndex++) {
		Sector_t* thisSector = sectors + sIndex;
		if (thisSector->numLineDefs) {
			//Nonzero number of lines.
			r_drawSector(thisSector, resolution, fbPTR);
			tmp = TRUE;
		}
	}
}



void r_initCamera(void) {
	camera = (Camera_t){
		.position=(Vec2f_t){.x=0.0f, .y=0.0f},
		.yaw=0.0f, .FOV=1.22173f, //70 degrees in radians
		.maxDistance=16.0f
	};
}



void r_createGeometry(void) {
	vertices[0] = (Vec2f_t){.x=-1.0f, .y=1.0f};
	vertices[1] = (Vec2f_t){.x=1.0f, .y=1.0f};
	vertices[2] = (Vec2f_t){.x=2.0f, .y=3.0f};
	vertices[3] = (Vec2f_t){.x=4.0f, .y=3.0f};

	lineDefs[0] = (LineDef_t){
		.vStart=0, .vEnd=1,
		.frontSector=0, .backSector=-1,
		.colour=RGB_RED
	};

	lineDefs[1] = (LineDef_t){
		.vStart=1, .vEnd=2,
		.frontSector=0, .backSector=-1,
		.colour=RGB_GREEN
	};

	lineDefs[2] = (LineDef_t){
		.vStart=2, .vEnd=3,
		.frontSector=0, .backSector=-1,
		.colour=RGB_BLUE
	};


	unsigned int* ldIndices;
	ldIndices = calloc(3, sizeof(unsigned int));
	ldIndices[0] = 0;
	ldIndices[1] = 1;
	ldIndices[2] = 2;
	sectors[0] = (Sector_t){
		.floorHeight=-1.0f, .ceilingHeight=1.0f,
		.lineDefs=ldIndices, .numLineDefs=3
	};
}