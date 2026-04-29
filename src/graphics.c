/* graphics.c */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "types.h"
#include "maths.h"
#include "terminal.h"
#include "bsp.h"



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

BSPnode_t BSPtreeRoot; //Root of the BSP tree.
//////// DATA ////////




//////// COLUMN DATA ////////
typedef uint8_t Depth_t;
Depth_t* depthMap; //1D depthmap.
unsigned int* lowYMap; //Lowest pixel of each column allowed to be drawn to
unsigned int* topYMap; //Highest pixel of each column allowed to be drawn to


void r_reallocColumnBuffers(const int width) {
	//If they exist, remove then remake.
	if (lowYMap) {free(lowYMap);}
	if (depthMap) {free(depthMap);}
	if (topYMap) {free(topYMap);}

	lowYMap = calloc(width, sizeof(unsigned int));
	depthMap = calloc(width, sizeof(Depth_t));
	topYMap = calloc(width, sizeof(unsigned int));
}


void r_clearColumnBuffers(const Vec2i_t resolution) {
	memset(lowYMap, (unsigned int)(0x00u), resolution.x * sizeof(Depth_t)); //Reset to all 0x00 (0px, bottom of the screen) values.
	memset(depthMap, (Depth_t)(0xFFu), resolution.x * sizeof(Depth_t)); //Reset to all 0xFF (255, max depth) values.
	for (unsigned int* ptr=topYMap; ptr<(topYMap+resolution.x); ptr++) {*ptr = resolution.y-1; /* Set to all [resY] values. */}
}


Depth_t r_mapDepth(float depthF) {
	return (Depth_t)(
		fmin(depthF / camera.maxDistance, 1.0f) * 255.0f //Remap to 0-255.
	);
}
//////// COLUMN DATA ////////



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


void r_drawSolidColumn(
	const int sectorID, int x, float invDistance,
	RGB_t* fbPTR, const Vec2i_t resolution, RGB_t colour
) {
	Depth_t mappedDepth = r_mapDepth(1.0f / invDistance);
	if (depthMap[x] <= mappedDepth) {return; /* Occluded */}

	//Check column maps;
	int minYBound = lowYMap[x];
	int maxYBound = topYMap[x];
	if (minYBound == maxYBound) {return; /* Column is full */}

	//Draw this wall collumn.
	int lowYBound, topYBound; //Area this column should span (inside segment)
	const Sector_t* thisSector = sectors + sectorID;
	r_getLineDefSectorProjections(
		thisSector, invDistance, &lowYBound, &topYBound, resolution
	);
	if ((topYBound<minYBound) || (lowYBound>=maxYBound)) {return; /* Completely offscreen vertically. */}

	int yLow = fmax(lowYBound, minYBound);
	int yTop = fmin(topYBound, maxYBound);

	//Column is taken, column was solid.
	lowYMap[x] = 0;
	topYMap[x] = 0;



#ifdef DEBUG_BORDERS
	//Draw ceiling border.
	*(fbPTR + x + (resolution.x * yLow)) = RGB_RED;

	//Draw floor border.
	*(fbPTR + x + (resolution.x * yTop)) = RGB_RED;

#else

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
	depthMap[x] = mappedDepth;

	//Draw floor.
	ptr = fbPTR + x + (resolution.x * yTop);
	for (int y=yTop; y<maxYBound; y++) {
		*ptr = thisSector->floorColour;
		ptr += resolution.x;	
	}
#endif
}


void r_drawPortalColumn(
	const int closeSectorID, const int farSectorID,
	int x, float invDistance,
	RGB_t* fbPTR, const Vec2i_t resolution, RGB_t colour
) {
	Depth_t mappedDepth = r_mapDepth(1.0f / invDistance);
	if (depthMap[x] <= mappedDepth) {return; /* Occluded */}

	//Check column maps;
	int minYBound = lowYMap[x];
	int maxYBound = topYMap[x];
	if (minYBound == maxYBound) {return; /* Column is full */}

	//Draw the ceiling, top part of the wall (if relevant), lower part (if relevant), and floor.
	//Close sector's projections;
	int lowYBoundNear, topYBoundNear;
	const Sector_t* nearSector = sectors + closeSectorID;
	r_getLineDefSectorProjections(
		nearSector, invDistance, &lowYBoundNear, &topYBoundNear, resolution
	);
	if ((topYBoundNear<minYBound) || (lowYBoundNear>=maxYBound)) {return; /* Completely offscreen vertically. */}
	lowYBoundNear = fmax(lowYBoundNear, 0);
	topYBoundNear = fmin(topYBoundNear, resolution.y-1);

	//Far sector's projections;
	int lowYBoundFar, topYBoundFar;
	const Sector_t* farSector = sectors + farSectorID;
	r_getLineDefSectorProjections(
		farSector, invDistance, &lowYBoundFar, &topYBoundFar, resolution
	);
	if ((topYBoundFar<minYBound) || (lowYBoundFar>=maxYBound)) {return; /* Completely offscreen vertically. */}
	lowYBoundFar = fmax(lowYBoundFar, 0);
	topYBoundFar = fmin(topYBoundFar, resolution.y-1);


	int yLow = fmin(lowYBoundNear, lowYBoundFar);
	int yTop = fmax(topYBoundNear, topYBoundFar);



#ifdef DEBUG_BORDERS
	//Draw lower border
	if (lowYBoundNear < lowYBoundFar) {
		//Draw a connecting wall between them and fill Y fill data.
		lowYMap[x] = lowYBoundFar;
		*(fbPTR + x + (resolution.x * lowYBoundFar)) = RGB_MAGENTA;
		*(fbPTR + x + (resolution.x * lowYBoundNear)) = RGB_BLUE;
	} else {
		//Just fill Y fill data.
		lowYMap[x] = lowYBoundNear;
		*(fbPTR + x + (resolution.x * lowYBoundNear)) = RGB_MAGENTA;
		*(fbPTR + x + (resolution.x * lowYBoundFar)) = RGB_BLUE;
	}

	//Draw upper border.
	if (topYBoundNear > topYBoundFar) {
		//Draw a connecting wall between them and fill Y fill data.
		topYMap[x] = topYBoundFar;
		*(fbPTR + x + (resolution.x * topYBoundFar)) = RGB_MAGENTA;
		*(fbPTR + x + (resolution.x * topYBoundNear)) = RGB_BLUE;
	} else {
		//Just fill Y fill data.
		topYMap[x] = topYBoundNear;
		*(fbPTR + x + (resolution.x * topYBoundNear)) = RGB_MAGENTA;
		*(fbPTR + x + (resolution.x * topYBoundFar)) = RGB_BLUE;
	}


#else

	//Draws top-to-bottom vertically.
	RGB_t* ptr;


	//Draw the ceiling
	ptr = fbPTR + x + (resolution.x * minYBound);
	for (int y=minYBound; y<yLow; y++) {
		*ptr = nearSector->ceilingColour;
		ptr += resolution.x;	
	}

	//Draw the lower (Y value, higher onscreen) section of the portal
	if (lowYBoundNear < lowYBoundFar) {
		//Draw a connecting wall between them and fill Y fill data.
		lowYMap[x] = lowYBoundFar;
		ptr = fbPTR + x + (resolution.x * lowYBoundNear);
		for (int y=lowYBoundNear; y<lowYBoundFar; y++) {
			*ptr = colour;
			ptr += resolution.x;
		}
	} else {
		//Just fill Y fill data.
		lowYMap[x] = lowYBoundNear;
		ptr = fbPTR + x + (resolution.x * lowYBoundFar);
		for (int y=lowYBoundFar; y<lowYBoundNear; y++) {
			*ptr = nearSector->ceilingColour;
			ptr += resolution.x;
		}
	}


	//Draw the upper (Y value, lower onscreen) section of the portal
	if (topYBoundNear > topYBoundFar) {
		//Draw a connecting wall between them and fill Y fill data.
		topYMap[x] = topYBoundFar;
		ptr = fbPTR + x + (resolution.x * topYBoundFar);
		for (int y=topYBoundFar; y<topYBoundNear; y++) {
			*ptr = colour;
			ptr += resolution.x;
		}
	} else {
		//Just fill Y fill data.
		topYMap[x] = topYBoundNear;
		ptr = fbPTR + x + (resolution.x * topYBoundNear);
		for (int y=topYBoundNear; y<topYBoundFar; y++) {
			*ptr = nearSector->floorColour;
			ptr += resolution.x;
		}
	}


	//Draw the floor
	ptr = fbPTR + x + (resolution.x * yTop);
	for (int y=yTop; y<maxYBound; y++) {
		*ptr = nearSector->floorColour;
		ptr += resolution.x;
	}
#endif
}




void r_drawSegment(const Segment_t thisSegment, const Vec2i_t resolution, RGB_t* fbPTR) {
	//Interpolate from start-end along the Segment.
	Vec2f_t start = thisSegment.vStart;
	Vec2f_t end = thisSegment.vEnd;

	float dStart = v2f_dot(camera.forward, v2f_sub(start, camera.position));
	float dEnd = v2f_dot(camera.forward, v2f_sub(end, camera.position));
    Vec2f_t dir = v2f_sub(end, start);
	Vec2f_t normal = (Vec2f_t){.x=-dir.y, .y=dir.x};

	if ((dStart < 0.0f) && (dEnd < 0.0f)) {return;}

	//If one point is behind, clip it.
	if ((dStart < 0.0f) || (dEnd < 0.0f)) {
	    float t = dStart / (dStart - dEnd);

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


	LineDef_t* thisLineDef = lineDefs + thisSegment.lineDefIndex;
	int closeSectorID, farSectorID, isSolid;
	if (thisLineDef->backSector == -1) {closeSectorID = thisLineDef->frontSector; isSolid=TRUE;}
	else if (thisLineDef->frontSector == -1) {closeSectorID = thisLineDef->backSector; isSolid=TRUE;}
	else {
		//Find which one is closer, assuming the normal points "front".
		isSolid = FALSE;
		float dotProd = v2f_dot(normal, v2f_sub(camera.position, start));
		if (dotProd >= 0.0f) {
			//Use "Front" sector.
			closeSectorID = thisLineDef->frontSector;
			farSectorID = thisLineDef->backSector;
		} else {
			//"Back" sector.
			closeSectorID = thisLineDef->backSector;
			farSectorID = thisLineDef->frontSector;
		}
	}


	//Draw, interpolating.
	float aspectRatio = (float)(resolution.x) / (float)(resolution.y);
	for (int x=leftMostClamp; x<rightMostClamp; x++) {
		float t = (float)(x - leftMost) / (float)(range);
		float invDistance = f_lerp(lInvDepth, rInvDepth, t);

		float a = (v2f_dot(v2f_normalise(normal), camera.forward) * 0.5f) + 0.5f;
		RGB_t colour = (RGB_t) {
			.r=thisLineDef->colour.r*a,
			.g=thisLineDef->colour.g*a,
			.b=thisLineDef->colour.b*a
		};
		t_quantise(&colour);
		if (isSolid) {
			r_drawSolidColumn(
				closeSectorID,
				x, aspectRatio*invDistance,
				fbPTR, resolution, colour
			);
		} else {
			r_drawPortalColumn(
				closeSectorID, farSectorID,
				x, aspectRatio*invDistance,
				fbPTR, resolution, colour
			);
		}
	}
}



void r_drawFrame(const Vec2i_t resolution) {
	RGB_t* fbPTR = t_getFramebufferPTR();
	r_clearColumnBuffers(resolution); //Reset depth data & column bottom/top data for this frame.

	/*
	for (unsigned int ldIndex=0u; ldIndex<MAX_LINEDEFS; ldIndex++) {
		LineDef_t* thisLineDef = lineDefs + ldIndex;
		if (!(thisLineDef->isValid)) {continue;}
		r_drawSegment(
			(Segment_t){
				.vStart=vertices[thisLineDef->vStart],
				.vEnd=vertices[thisLineDef->vEnd],
				.lineDefIndex=ldIndex
			},
			resolution, fbPTR	
		);
	}*/

	bsp_walk(
		&BSPtreeRoot, camera.position, //BSP parameters
		resolution, fbPTR, //Function parameters
		r_drawSegment, //Function
		0u //Start depth
	);
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



int r_createGeometry(void) {
	vertices[0] = (Vec2f_t){.x=-10.0f, .y= 10.0f};
	vertices[1] = (Vec2f_t){.x= 10.0f, .y= 10.0f};
	vertices[2] = (Vec2f_t){.x= 15.0f, .y=-10.0f};
	vertices[3] = (Vec2f_t){.x=-17.5f, .y=-10.0f};
	vertices[4] = (Vec2f_t){.x=-20.0f, .y=  0.0f};
	vertices[5] = (Vec2f_t){.x=-30.0f, .y=-10.0f};
	vertices[6] = (Vec2f_t){.x=-25.0f, .y=  0.0f};
	vertices[7] = (Vec2f_t){.x= 15.0f, .y= 10.0f};



	lineDefs[0] = (LineDef_t){
		.vStart=0, .vEnd=1,
		.frontSector=0, .backSector=-1,
		.colour=RGB_RED,
		.isValid=TRUE
	};

	lineDefs[1] = (LineDef_t){
		.vStart=1, .vEnd=2,
		.frontSector=2, .backSector=0,
		.colour=RGB_CYAN,
		.isValid=TRUE
	};

	lineDefs[2] = (LineDef_t){
		.vStart=2, .vEnd=3,
		.frontSector=0, .backSector=-1,
		.colour=RGB_RED,
		.isValid=TRUE
	};

	lineDefs[3] = (LineDef_t){
		.vStart=3, .vEnd=4,
		.frontSector=1, .backSector=0,
		.colour=RGB_YELLOW,
		.isValid=TRUE
	};

	lineDefs[4] = (LineDef_t){
		.vStart=4, .vEnd=0,
		.frontSector=0, .backSector=-1,
		.colour=RGB_RED,
		.isValid=TRUE
	};

	lineDefs[5] = (LineDef_t){
		.vStart=3, .vEnd=5,
		.frontSector=1, .backSector=-1,
		.colour=RGB_BLUE,
		.isValid=TRUE
	};

	lineDefs[6] = (LineDef_t){
		.vStart=5, .vEnd=6,
		.frontSector=1, .backSector=-1,
		.colour=RGB_BLUE,
		.isValid=TRUE
	};

	lineDefs[7] = (LineDef_t){
		.vStart=6, .vEnd=4,
		.frontSector=1, .backSector=-1,
		.colour=RGB_BLUE,
		.isValid=TRUE
	};

	lineDefs[8] = (LineDef_t){
		.vStart=1, .vEnd=7,
		.frontSector=2, .backSector=-1,
		.colour=RGB_YELLOW,
		.isValid=TRUE
	};

	lineDefs[9] = (LineDef_t){
		.vStart=7, .vEnd=2,
		.frontSector=2, .backSector=-1,
		.colour=RGB_YELLOW,
		.isValid=TRUE
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



	unsigned int* ldIndicesSector2;
	ldIndicesSector2 = calloc(3, sizeof(unsigned int));
	ldIndicesSector2[0] = 1;
	ldIndicesSector2[1] = 8;
	ldIndicesSector2[2] = 9;
	sectors[2] = (Sector_t){
		.floorHeight=-0.5f, .floorColour=RGB_GREEN,
		.ceilingHeight=0.5f, .ceilingColour=RGB_WHITE,
		.lineDefs=ldIndicesSector2, .numLineDefs=3
	};


	int success;
	BSPtreeRoot = bsp_build(
		vertices, 8,
		lineDefs, 10,
		&success
	);
	return success;
}


void r_freeBSPTree(void) {
	bsp_free(&BSPtreeRoot);
}
//////// INITIALISATION ////////


