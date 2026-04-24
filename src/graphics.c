/* graphics.c */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "types.h"
#include "maths.h"
#include "terminal.h"



//////// DATA ////////
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
//////// DATA ////////




//////// DEPTH MAPPING ////////
typedef uint8_t Depth_t;
Depth_t* depthMap; //1D depthmap.

void r_reallocDepthMap(const int width) {
	if (depthMap) {
		free(depthMap);
	}
	depthMap = calloc(width, sizeof(Depth_t));
}


void r_clearDepth(const int width) {
	memset(depthMap, (Depth_t)(0xFFu), width * sizeof(Depth_t)); //Reset to all 0xFF (255, max depth) values.
}


Depth_t r_mapDepth(float depthF) {
	return (Depth_t)(
		fmin(depthF / camera.maxDistance, 1.0f) * 255.0f //Remap to 0-255.
	);
}
//////// DEPTH MAPPING ////////



//////// DRAWING ////////
int r_getCentreX(const Vec2f_t position, const Vec2i_t resolution) {
	Vec2f_t direction = v2f_sub(position, camera.position);

	float theta = atan2(direction.x, direction.y);
	float angleDelta = theta - camera.yaw;

	while (angleDelta >  M_PI) angleDelta -= 2.0f * M_PI;
	while (angleDelta < -M_PI) angleDelta += 2.0f * M_PI;

	float centreX = ((float)(resolution.x) / 2.0f) * ((angleDelta * 2.0f / camera.FOV) + 1.0f);
	return (int)(centreX);
}


void r_getLineDefSectorProjections(
	const Sector_t* thisSector, float invDistance, int* lowYBound, int* topYBound, const Vec2i_t resolution
) {
	float projectedYFloor = (cameraZ - thisSector->floorHeight) * invDistance;
	float projectedYCeiling = (cameraZ - thisSector->ceilingHeight) * invDistance;

	*lowYBound = (int)((float)(resolution.y) * (0.5f - projectedYFloor));
	*topYBound = (int)((float)(resolution.y) * (0.5f - projectedYCeiling));
}


void r_drawColumn(
	const int sectorID,
	int x, float invDistance,
	RGB_t* fbPTR, const Vec2i_t resolution, RGB_t colour,
	const int minYBound, const int maxYBound //Range allowed to draw in.
) {
	//Draw this wall collumn.
	int lowYBound, topYBound;
	const Sector_t* thisSector = sectors + sectorID;
	r_getLineDefSectorProjections(
		thisSector, invDistance, &lowYBound, &topYBound, resolution
	);
	if ((topYBound<minYBound) || (lowYBound>=maxYBound)) {return; /* Completely offscreen vertically. */}

	int yLow = fmax(lowYBound, minYBound);
	int yTop = fmin(topYBound, maxYBound);

	//Draws top-to-bottom vertically.
	RGB_t* ptr;

	//Draw ceiling.
	ptr = fbPTR + x + (resolution.x * minYBound);
	for (int y=minYBound; y<yLow; y++) {
		*ptr = thisSector->ceilingColour;
		ptr += resolution.x;	
	}

	//Draw wall.
	ptr = fbPTR + x + (resolution.x * yLow);
	for (int y=yLow; y<yTop; y++) {
		//Texturing TBA. yLow is low INDEX not low ONSCREEN.
		*ptr = colour; //Magenta for now.
		ptr += resolution.x;
	}

	//Draw floor.
	ptr = fbPTR + x + (resolution.x * yTop);
	for (int y=yTop; y<maxYBound; y++) {
		*ptr = thisSector->floorColour;
		ptr += resolution.x;	
	}
}



void r_drawLineDef(
	const LineDef_t* thisLineDef, const Vec2i_t resolution, RGB_t* fbPTR,
	const int minYBound, const int maxYBound, //Range allowed to draw in.
	const int sectorID
) {
	//Interpolate from start-end along the LineDef.
	Vec2f_t start = vertices[thisLineDef->vStart];
	Vec2f_t end = vertices[thisLineDef->vEnd];


	float dStart = v2f_dot(camera.forward, v2f_sub(start, camera.position));
	float dEnd = v2f_dot(camera.forward, v2f_sub(end, camera.position));

	if ((dStart < 0.0f) && (dEnd < 0.0f)) return;

	//If one point is behind, clip it.
	if ((dStart < 0.0f) || (dEnd < 0.0f)) {
	    float t = dStart / (dStart - dEnd);

	    Vec2f_t dir = v2f_sub(end, start);
	    Vec2f_t clipPoint = v2f_add(start, v2f_mul(dir, t));

	    if (dStart < 0.0f) {start = clipPoint;}
	    else {end = clipPoint;}
	}


	//Project into screen horizontally
	int startX = r_getCentreX(start, resolution);
	int endX = r_getCentreX(end, resolution);
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

	int leftMostClamp = fmax(leftMost, 0);
	int rightMostClamp = fmin(rightMost, resolution.x);
	if ((rightMostClamp < 0) || (leftMostClamp >= resolution.x)) {return; /* Offscreen horizontally */}


	//Draw, interpolating.
	float aspectRatio = (float)(resolution.x) / (float)(resolution.y);
	for (int x=leftMostClamp; x<rightMostClamp; x++) {
		float t = (float)(x - leftMost) / (float)(range);
		float invDistance = f_lerp(lInvDepth, rInvDepth, t);

		Depth_t mappedDepth = r_mapDepth(1.0f / invDistance);
		if (depthMap[x] <= mappedDepth) {continue; /* Occluded */}
		depthMap[x] = mappedDepth;

		RGB_t colour = (RGB_t) {
			.r=thisLineDef->colour.r*t,
			.g=thisLineDef->colour.g*t,
			.b=thisLineDef->colour.b*t
		};
		t_quantise(&colour);
		r_drawColumn(
			sectorID,
			x, aspectRatio * invDistance,
			fbPTR, resolution, colour,
			minYBound, maxYBound
		);
	}
}


void r_drawSector(
	const Sector_t* thisSector, const Vec2i_t resolution, RGB_t* fbPTR,
	const int minYBound, const int maxYBound, //Range allowed to draw in.
	const int sectorID
) {
	//Loop through it's linedefs, drawing those.
	for (unsigned int secLD=0u; secLD<thisSector->numLineDefs; secLD++) {
		unsigned int ldIndex = thisSector->lineDefs[secLD];
		const LineDef_t* thisLineDef = lineDefs + ldIndex;
		r_drawLineDef(
			thisLineDef, resolution, fbPTR,
			minYBound, maxYBound,
			sectorID
		);
	}
}



void r_drawFrame(const Vec2i_t resolution) {
	RGB_t* fbPTR = t_getFramebufferPTR();
	r_clearDepth(resolution.x); //Reset depth data for this frame.

	for (unsigned int sIndex=0u; sIndex<MAX_SECTORS; sIndex++) {
		Sector_t* thisSector = sectors + sIndex;
		if (thisSector->numLineDefs) {
			//Nonzero number of lines.
			r_drawSector(
				thisSector, resolution, fbPTR,
				0, resolution.y-1,
				sIndex
			);
		}
	}
}
//////// DRAWING ////////



//////// INITIALISATION ////////
void r_initCamera(void) {
	camera = (Camera_t){
		.position=(Vec2f_t){.x=0.0f, .y=0.0f},
		.yaw=0.0f, .FOV=1.22173f, //70 degrees in radians
		.maxDistance=32.0f,
		.forward=(Vec2f_t){.x=0.0f, .y=1.0f}
	};
}



void r_createGeometry(void) {
	vertices[0] = (Vec2f_t){.x=-10.0f, .y= 10.0f};
	vertices[1] = (Vec2f_t){.x= 10.0f, .y= 10.0f};
	vertices[2] = (Vec2f_t){.x= 15.0f, .y=-10.0f};
	vertices[3] = (Vec2f_t){.x=-17.5f, .y=-10.0f};
	vertices[4] = (Vec2f_t){.x=-20.0f, .y=  0.0f};
	vertices[5] = (Vec2f_t){.x=-30.0f, .y=-10.0f};
	vertices[6] = (Vec2f_t){.x=-25.0f, .y=  0.0f};



	lineDefs[0] = (LineDef_t){
		.vStart=0, .vEnd=1,
		.frontSector=0, .backSector=-1,
		.colour=RGB_RED
	};

	lineDefs[1] = (LineDef_t){
		.vStart=1, .vEnd=2,
		.frontSector=0, .backSector=-1,
		.colour=RGB_CYAN
	};

	lineDefs[2] = (LineDef_t){
		.vStart=2, .vEnd=3,
		.frontSector=0, .backSector=-1,
		.colour=RGB_RED
	};

	lineDefs[3] = (LineDef_t){
		.vStart=3, .vEnd=4,
		.frontSector=0, .backSector=1,
		.colour=RGB_YELLOW
	};

	lineDefs[4] = (LineDef_t){
		.vStart=4, .vEnd=0,
		.frontSector=0, .backSector=-1,
		.colour=RGB_RED
	};

	lineDefs[5] = (LineDef_t){
		.vStart=3, .vEnd=5,
		.frontSector=1, .backSector=-1,
		.colour=RGB_BLUE
	};

	lineDefs[6] = (LineDef_t){
		.vStart=5, .vEnd=6,
		.frontSector=1, .backSector=-1,
		.colour=RGB_BLUE
	};

	lineDefs[7] = (LineDef_t){
		.vStart=6, .vEnd=4,
		.frontSector=1, .backSector=-1,
		.colour=RGB_BLUE
	};



	unsigned int* ldIndicesSector0;
	ldIndicesSector0 = calloc(5, sizeof(unsigned int));
	ldIndicesSector0[0] = 0;
	ldIndicesSector0[1] = 1;
	ldIndicesSector0[2] = 2;
	ldIndicesSector0[3] = 3;
	ldIndicesSector0[4] = 4;
	sectors[0] = (Sector_t){
		.floorHeight=-2.0f, .floorColour=RGB_RED,
		.ceilingHeight=2.0f, .ceilingColour=RGB_WHITE,
		.lineDefs=ldIndicesSector0, .numLineDefs=5
	};



	unsigned int* ldIndicesSector1;
	ldIndicesSector1 = calloc(4, sizeof(unsigned int));
	ldIndicesSector1[0] = 3;
	ldIndicesSector1[1] = 5;
	ldIndicesSector1[2] = 6;
	ldIndicesSector1[3] = 7;
	sectors[1] = (Sector_t){
		.floorHeight=-3.0f, .floorColour=RGB_BLUE,
		.ceilingHeight=3.0f, .ceilingColour=RGB_WHITE,
		.lineDefs=ldIndicesSector1, .numLineDefs=4
	};
}
//////// INITIALISATION ////////


