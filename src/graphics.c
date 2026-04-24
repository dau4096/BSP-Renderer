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
	for (unsigned int* ptr=topYMap; ptr<(topYMap+resolution.y); ptr++) {*ptr = resolution.y; /* Set to all [resY] values. */}
}


Depth_t r_mapDepth(float depthF) {
	return (Depth_t)(
		fmin(depthF / camera.maxDistance, 1.0f) * 255.0f //Remap to 0-255.
	);
}


int r_manageColumnValues(unsigned int x, unsigned int* lowYBound, unsigned int* topYBound) { //Returns success.
	//Check column is writeable
	if (lowYMap[x] >= topYMap[x]) {return FALSE; /* Column is already full. */}

	//Replace values in the maps.
	*lowYBound = MAX(*lowYBound, lowYMap[x]);
	*topYBound = MAX(*topYBound, topYMap[x]);

	return TRUE; //Success.
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
	const Sector_t* thisSector, float invDistance, int* lowY, int* topY, const Vec2i_t resolution
) {
	float projectedYFloor = (cameraZ - thisSector->floorHeight) * invDistance;
	float projectedYCeiling = (cameraZ - thisSector->ceilingHeight) * invDistance;

	*lowY = (int)((float)(resolution.y) * (0.5f - projectedYFloor));
	*topY = (int)((float)(resolution.y) * (0.5f - projectedYCeiling));
}


void r_drawColumn(
	const int sectorID, int x, float invDistance,
	RGB_t* fbPTR, const Vec2i_t resolution, RGB_t colour
) {
	//Draw this wall collumn.
	int lowY, topY; //Area this column should span (inside segment)
	const Sector_t* thisSector = sectors + sectorID;
	r_getLineDefSectorProjections(
		thisSector, invDistance, &lowY, &topY, resolution
	);

	int lowYBound=lowYMap[x], topYBound=topYMap[x]; //Extents allowed to fill.
	int success = r_manageColumnValues(x, &lowY, &topY);
	if (!success) {return; /* Column is full. */}


	//Draws top-to-bottom vertically.
	RGB_t* ptr;
	lowY = fmax(lowY, 0);
	topY = fmin(topY, resolution.y);

	//Draw ceiling.
	ptr = fbPTR + x + (resolution.x * lowYBound);
	for (int y=lowYBound; y<lowY; y++) {
		*ptr = thisSector->ceilingColour;
		ptr += resolution.x;
	}

	//Draw wall.
	ptr = fbPTR + x + (resolution.x * lowY);
	for (int y=lowY; y<topY; y++) {
		//Texturing TBA. lowY is low INDEX not low ONSCREEN.
		*ptr = colour; //Magenta for now.
		ptr += resolution.x;
	}

	//Draw floor.
	ptr = fbPTR + x + (resolution.x * topY);
	for (int y=topY; y<topYBound; y++) {
		*ptr = thisSector->floorColour;
		ptr += resolution.x;	
	}
}




void r_drawSegment(const Segment_t thisSegment, const Vec2i_t resolution, RGB_t* fbPTR) {
	//Interpolate from start-end along the Segment.
	Vec2f_t start = thisSegment.vStart;
	Vec2f_t end = thisSegment.vEnd;


	float dStart = v2f_dot(camera.forward, v2f_sub(start, camera.position));
	float dEnd = v2f_dot(camera.forward, v2f_sub(end, camera.position));

	if ((dStart < 0.0f) && (dEnd < 0.0f)) {return;}

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
	for (int x=leftMostClamp; x<rightMostClamp; x++) {
		float t = (float)(x - leftMost) / (float)(range);
		float invDistance = f_lerp(lInvDepth, rInvDepth, t);
		float depthF = 1.0f / invDistance;

		//Check column depth
		Depth_t mappedDepth = r_mapDepth(depthF);
		if (depthMap[x] <= mappedDepth) {continue; /* Nearer than current depth. */}

		LineDef_t* thisLineDef = lineDefs + thisSegment.lineDefIndex;
		RGB_t colour = (RGB_t) {
			.r=thisLineDef->colour.r*t,
			.g=thisLineDef->colour.g*t,
			.b=thisLineDef->colour.b*t
		};
		t_quantise(&colour);
		r_drawColumn(
			thisLineDef->frontSector,
			x, invDistance,
			fbPTR, resolution, colour
		);
	}
}



void r_drawFrame(const Vec2i_t resolution) {
	RGB_t* fbPTR = t_getFramebufferPTR();
	r_clearColumnBuffers(resolution); //Reset depth data & column bottom/top data for this frame.

	bsp_walk(
		&BSPtreeRoot, camera.position, //BSP parameters
		resolution, fbPTR, //Function parameters
		r_drawSegment //Function
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


	int success;
	BSPtreeRoot = bsp_build(
		vertices, MAX_VERTICES,
		lineDefs, MAX_LINEDEFS,
		&success
	);
	return success;
}


void r_freeBSPTree(void) {
	bsp_free(&BSPtreeRoot);
}
//////// INITIALISATION ////////


