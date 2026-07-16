/* graphics.c */
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#include "graphics.h"

#include "types.h"
#include "maths.h"
#include "terminal.h"



//////// DATA ////////
Camera_t camera; //The view
Camera_t* r_camera = &camera; //Ptr to ^^


unsigned int g_numVertices;
unsigned int g_numLineDefs;
unsigned int g_numSectors;

Vec2f_t* g_vertices; //2D positions.
LineDef_t* g_lineDefs; //LineDefs connecting vertices.
Sector_t* g_sectors; //Sectors made of LineDefs.
//////// DATA ////////



//////// CONSTANTS ////////
#define EPSILON 1.0e-3f
#define NEAR_PLANE 1.0e-3f
//////// CONSTANTS ////////



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
	for (unsigned int* ptr=topYMap; ptr<(topYMap+resolution.x); ptr++) {*ptr = resolution.y; /* Set to all [resY] values. */}
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




//////// TEXTURES ////////
#define TEXTURE_RESOLUTION ((Vec2i_t){.x=32, .y=32})
RGB_t* textures[MAX_TEXTURES]; //Stores texture data. Each entry is a 32×32 grid of pixel data (1D) organised by columns ([(x * 32) + y])



int r_loadTexture(const char* path, RGB_t** pixels) { //Returns success
	stbi_set_flip_vertically_on_load(TRUE);
	int width, height, channels;
	unsigned char* textureDataSTBI = stbi_load(
		path,
		&width, &height,
		&channels, 3 //Only take RGB back, not A.
	);

	if (
		(!textureDataSTBI) || //Failed to load
		(width != TEXTURE_RESOLUTION.x) || (height != TEXTURE_RESOLUTION.y) //Wrong resolution
	) {return FALSE; /* Invalid */}

	//Transpose, and convert from [unsigned char] data to (RGB_t)[R, G, B] data.
	//Pixel data is used in columns so swapping from [y][x] order to [x][y] order is worthwhile.
	*pixels = malloc(sizeof(RGB_t) * width * height);
	for (unsigned int i=0u; i<TEXTURE_RESOLUTION.x; i++) {
		for (unsigned int j=0u; j<TEXTURE_RESOLUTION.y; j++) {
			unsigned char* pxStart = textureDataSTBI + ((j * TEXTURE_RESOLUTION.x) + i) * 3u;
			RGB_t* ptr = (*pixels) + (i * TEXTURE_RESOLUTION.y) + j;
			*ptr = (RGB_t){
				.r=(uint8_t)(*(pxStart+0)),
				.g=(uint8_t)(*(pxStart+1)),
				.b=(uint8_t)(*(pxStart+2))
			};
			rgb_quantise(ptr);
		}
	}

	//Cleanup
	stbi_image_free(textureDataSTBI);

	return TRUE;
}


int r_getColumn(const unsigned int ID, int x, RGB_t** ptr) { //Returns success
	if (ID >= MAX_TEXTURES) {return FALSE; /* Invalid */}
	if (x < 0) {x = 0;}
	else if (x >= TEXTURE_RESOLUTION.y) {x = TEXTURE_RESOLUTION.x-1;}
	*ptr = textures[ID] + (x * TEXTURE_RESOLUTION.y); //Organised in columns, so iterate over this [TEXTURE_RESOLUTION.y] times for a full column.
	return TRUE;
}

//////// TEXTURES ////////





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
	float projectedYFloor = (camera.Z - thisSector->floorHeight) * invDistance;
	float projectedYCeiling = (camera.Z - thisSector->ceilingHeight) * invDistance;

	*lowY = (int)((float)(resolution.y) * (0.5f - projectedYFloor));
	*topY = (int)((float)(resolution.y) * (0.5f - projectedYCeiling));
}


void r_drawSolidColumn(
	const int sectorID, int screenX, float invDistance, int textureX,
	RGB_t* fbPTR, const Vec2i_t resolution, unsigned int textureID
) {
	Depth_t mappedDepth = r_mapDepth(1.0f / invDistance);
	if (depthMap[screenX] <= mappedDepth) {return; /* Occluded */}

	//Check column maps;
	int minYBound = lowYMap[screenX];
	int maxYBound = topYMap[screenX];
	if (minYBound == maxYBound) {return; /* Column is full */}

	//Draw this wall collumn.
	int lowYBound, topYBound; //Area this column should span (inside segment)
	const Sector_t* thisSector = g_sectors + sectorID;
	r_getLineDefSectorProjections(
		thisSector, invDistance, &lowYBound, &topYBound, resolution
	);
	if ((topYBound<minYBound) || (lowYBound>=maxYBound)) {return; /* Completely offscreen vertically. */}

	int yLow = fmax(lowYBound, minYBound);
	int yTop = fmin(topYBound, maxYBound);

	//Column is taken, column was solid.
	lowYMap[screenX] = 0;
	topYMap[screenX] = 0;



#ifdef DEBUG_BORDERS
	//Draw ceiling border.
	*(fbPTR + screenX + (resolution.x * yLow)) = RGB_RED;

	//Draw floor border.
	*(fbPTR + screenX + (resolution.x * yTop)) = RGB_RED;

#else

	//Draws top-to-bottom vertically. (Image is flipped when drawing to console)
	RGB_t* ptr;

	//Draw ceiling.
	ptr = fbPTR + screenX + (resolution.x * minYBound);
	RGB_t floorColour = rgb_umul(thisSector->floorColour, thisSector->lightLevel);
	for (int y=minYBound; y<yLow; y++) {
		*ptr = floorColour;
		ptr += resolution.x;
	}

	//Draw wall.
	ptr = fbPTR + screenX + (resolution.x * yLow);
	RGB_t* texPTR;
	if (!r_getColumn(textureID, textureX, &texPTR)) {return;}
	for (int y=yLow; y<yTop; y++) {
		float t = (float)(y - lowYBound) / (float)(topYBound - lowYBound);
		*ptr = rgb_umul(*(texPTR + (int)(t * (float)(TEXTURE_RESOLUTION.y))), thisSector->lightLevel);
		ptr += resolution.x;
	}
	depthMap[screenX] = mappedDepth;

	//Draw floor.
	ptr = fbPTR + screenX + (resolution.x * yTop);
	RGB_t ceilingColour = rgb_umul(thisSector->ceilingColour, thisSector->lightLevel);
	for (int y=yTop; y<maxYBound; y++) {
		*ptr = ceilingColour;
		ptr += resolution.x;	
	}
#endif
}


void r_drawPortalColumn(
	const int closeSectorID, const int farSectorID,
	int screenX, float invDistance, int textureX,
	RGB_t* fbPTR, const Vec2i_t resolution, unsigned int textureID
) {
	Depth_t mappedDepth = r_mapDepth(1.0f / invDistance);
	if (depthMap[screenX] <= mappedDepth) {return; /* Occluded */}

	//Check column maps;
	int minYBound = lowYMap[screenX];
	int maxYBound = topYMap[screenX];
	if (minYBound == maxYBound) {return; /* Column is full */}

	//Draw the ceiling, top part of the wall (if relevant), lower part (if relevant), and floor.
	//Close sector's projections;
	int lowYBoundNearUnclamp, topYBoundNearUnclamp;
	const Sector_t* nearSector = g_sectors + closeSectorID;
	r_getLineDefSectorProjections(
		nearSector, invDistance, &lowYBoundNearUnclamp, &topYBoundNearUnclamp, resolution
	);
	if ((topYBoundNearUnclamp<minYBound) || (lowYBoundNearUnclamp>=maxYBound)) {return; /* Completely offscreen vertically. */}
	int lowYBoundNear = fmax(lowYBoundNearUnclamp, minYBound);
	int topYBoundNear = fmin(topYBoundNearUnclamp, maxYBound);

	//Far sector's projections;
	int lowYBoundFarUnclamp, topYBoundFarUnclamp;
	const Sector_t* farSector = g_sectors + farSectorID;
	r_getLineDefSectorProjections(
		farSector, invDistance, &lowYBoundFarUnclamp, &topYBoundFarUnclamp, resolution
	);
	if ((topYBoundFarUnclamp<minYBound) || (lowYBoundFarUnclamp>=maxYBound)) {return; /* Completely offscreen vertically. */}
	int lowYBoundFar = fmax(lowYBoundFarUnclamp, minYBound);
	int topYBoundFar = fmin(topYBoundFarUnclamp, maxYBound);


	int yLow = fmin(lowYBoundNear, lowYBoundFar);
	int yTop = fmax(topYBoundNear, topYBoundFar);



#ifdef DEBUG_BORDERS
	//Draw lower border
	if (lowYBoundNear < lowYBoundFar) {
		//Draw a connecting wall between them and fill Y fill data.
		lowYMap[screenX] = lowYBoundFar;
		*(fbPTR + screenX + (resolution.x * lowYBoundFar)) = RGB_MAGENTA;
		*(fbPTR + screenX + (resolution.x * lowYBoundNear)) = RGB_BLUE;
	} else {
		//Just fill Y fill data.
		lowYMap[screenX] = lowYBoundNear;
		*(fbPTR + screenX + (resolution.x * lowYBoundNear)) = RGB_MAGENTA;
		*(fbPTR + screenX + (resolution.x * lowYBoundFar)) = RGB_BLUE;
	}

	//Draw upper border.
	if (topYBoundNear > topYBoundFar) {
		//Draw a connecting wall between them and fill Y fill data.
		topYMap[screenX] = topYBoundFar;
		*(fbPTR + screenX + (resolution.x * topYBoundFar)) = RGB_MAGENTA;
		*(fbPTR + screenX + (resolution.x * topYBoundNear)) = RGB_BLUE;
	} else {
		//Just fill Y fill data.
		topYMap[screenX] = topYBoundNear;
		*(fbPTR + screenX + (resolution.x * topYBoundNear)) = RGB_MAGENTA;
		*(fbPTR + screenX + (resolution.x * topYBoundFar)) = RGB_BLUE;
	}


#else

	//Draws top-to-bottom vertically.
	RGB_t* ptr;
	RGB_t floorColour = rgb_umul(nearSector->floorColour, nearSector->lightLevel);
	RGB_t ceilingColour = rgb_umul(nearSector->ceilingColour, nearSector->lightLevel);


	//Draw the ceiling
	ptr = fbPTR + screenX + (resolution.x * minYBound);
	for (int y=minYBound; y<yLow; y++) {
		*ptr = floorColour;
		ptr += resolution.x;
	}

	//Draw the lower (Y value, lower onscreen) section of the portal
	RGB_t* texPTR;
	if (!r_getColumn(textureID, textureX, &texPTR)) {return;}
	if (lowYBoundNear < lowYBoundFar) {
		//Draw a connecting wall between them and fill Y fill data.
		lowYMap[screenX] = lowYBoundFar;
		ptr = fbPTR + screenX + (resolution.x * lowYBoundNear);
		for (int y=lowYBoundNear; y<lowYBoundFar; y++) {
			float t = (float)(y - lowYBoundNearUnclamp) / (float)(lowYBoundFarUnclamp - lowYBoundNearUnclamp);
			*ptr = rgb_umul(*(texPTR + (int)(t * (float)TEXTURE_RESOLUTION.y)), nearSector->lightLevel);
			ptr += resolution.x;
		}
	} else {
		//Just fill Y fill data.
		lowYMap[screenX] = lowYBoundNear;
		ptr = fbPTR + screenX + (resolution.x * lowYBoundFar);
		for (int y=lowYBoundFar; y<lowYBoundNear; y++) {
			*ptr = floorColour;
			ptr += resolution.x;
		}
	}


	//Draw the upper (Y value, higher onscreen) section of the portal
	if (topYBoundNear > topYBoundFar) {
		//Draw a connecting wall between them and fill Y fill data.
		topYMap[screenX] = topYBoundFar;
		ptr = fbPTR + screenX + (resolution.x * topYBoundFar);
		for (int y=topYBoundFar; y<topYBoundNear; y++) {
			float t = (float)(y - topYBoundFarUnclamp) / (float)(topYBoundNearUnclamp - topYBoundFarUnclamp);
			*ptr = rgb_umul(*(texPTR + (int)(t * (float)TEXTURE_RESOLUTION.y)), nearSector->lightLevel);
			ptr += resolution.x;
		}
	} else {
		//Just fill Y fill data.
		topYMap[screenX] = topYBoundNear;
		ptr = fbPTR + screenX + (resolution.x * topYBoundNear);
		for (int y=topYBoundNear; y<topYBoundFar; y++) {
			*ptr = ceilingColour;
			ptr += resolution.x;
		}
	}


	//Draw the floor
	ptr = fbPTR + screenX + (resolution.x * yTop);
	for (int y=yTop; y<maxYBound; y++) {
		*ptr = ceilingColour;
		ptr += resolution.x;
	}
#endif
}





int r_clipLDVertices(Vec2f_t* start, Vec2f_t* end, float* startT, float* endT) {
	//Clip to the "View frustum" in 2D. (View trapezium, really.)
	//If it's out of the view entirely, return FALSE.
	
	float leftAngle = camera.yaw - (camera.FOV * 0.5f);
	Vec2f_t leftDirection = (Vec2f_t){.x=sin(leftAngle), .y=cos(leftAngle)}; //Points direction of leftside of screen.
	Vec2f_t leftNormal = (Vec2f_t){.x=leftDirection.y, .y=-leftDirection.x}; //Rotate CW to face into view. Positive values means RIGHT of leftmost edge.

	float rightAngle = camera.yaw + (camera.FOV * 0.5f);
	Vec2f_t rightDirection = (Vec2f_t){.x=sin(rightAngle), .y=cos(rightAngle)}; //Points direction of rightside of screen.
	Vec2f_t rightNormal = (Vec2f_t){.x=-rightDirection.y, .y=rightDirection.x}; //Rotate ACW to face into view. Positive values means LEFT of rightmost edge.


	Vec2f_t deltaStart = v2f_sub(*start, camera.position);
	Vec2f_t deltaEnd = v2f_sub(*end, camera.position);

	float leftDotStart = v2f_dot(leftNormal, deltaStart);
	float leftDotEnd = v2f_dot(leftNormal, deltaEnd);

	float rightDotStart = v2f_dot(rightNormal, deltaStart);
	float rightDotEnd = v2f_dot(rightNormal, deltaEnd);

	float forwardDotStart = v2f_dot(camera.forward, deltaStart);
	float forwardDotEnd = v2f_dot(camera.forward, deltaEnd);

	if (
		((leftDotStart < 0.0f) && (leftDotEnd < 0.0f)) || //Left
		((rightDotStart < 0.0f) && (rightDotEnd < 0.0f)) || //Right
		((forwardDotStart < 0.0f) && (forwardDotEnd < 0.0f))  //Behind
	) {
		//Outside of view - Entirely offscreen left, right, or behind.
		return FALSE;
	}


	if (
		(leftDotStart >= 0.0f) && (rightDotStart >= 0.0f) && 
		(leftDotEnd >= 0.0f) && (rightDotEnd >= 0.0f)
	) {
		//Both are in the view, exit early.
		*startT = 0.0f;
		*endT = 1.0f;
		return TRUE;
	}


	Vec2f_t originalStart = *start;
	Vec2f_t originalEnd = *end;
	Vec2f_t originalDelta = v2f_sub(*end, *start);
	

	//Handle start clipping.
	if (leftDotStart < 0.0f) {
		//Start is offscreen to the left
		float t = (-leftDotStart) / (fabsf(leftDotEnd) + fabsf(leftDotStart));
		*start = v2f_add(originalStart, v2f_mul(originalDelta, t));
		*startT = t;

	} else if (rightDotStart < 0.0f) {
		//Start is offscreen to the right (Cannot be both - Would be behind camera and so have exited by this point.)
		float t = (-rightDotStart) / (fabsf(rightDotEnd) + fabsf(rightDotStart));
		*start = v2f_add(originalStart, v2f_mul(originalDelta, t));
		*startT = t;

	}

	//Handle end clipping.
	if (leftDotEnd < 0.0f) {
		//End is offscreen to the left
		float t = (-leftDotEnd) / (fabsf(leftDotStart) + fabsf(leftDotEnd));
		*end = v2f_sub(originalEnd, v2f_mul(originalDelta, t));
		*endT = 1.0f - t;

	}
	if (rightDotEnd < 0.0f) {
		//End is offscreen to the right
		float t = (-rightDotEnd) / (fabsf(rightDotStart) + fabsf(rightDotEnd));
		*end = v2f_sub(originalEnd, v2f_mul(originalDelta, t));
		*endT = 1.0f - t;

	}

	return TRUE; //In the view.
}




void r_drawLineDef(const LineDef_t* thisLineDef, const Vec2i_t resolution, RGB_t* fbPTR) {
	//Interpolate from start-end along the Segment.
	Vec2f_t start = g_vertices[thisLineDef->vStart];
	Vec2f_t end = g_vertices[thisLineDef->vEnd];


	//Clip to the camera "frustum" (trapezium in 2D)
	//Stops projections getting too absurd.
	float startT = 0.0f, endT = 1.0f;
	if (!r_clipLDVertices(&start, &end, &startT, &endT)) {return; /* Both vertices are offscreen. */}


	float dStart = v2f_dot(camera.forward, v2f_sub(start, camera.position));
	float dEnd = v2f_dot(camera.forward, v2f_sub(end, camera.position));
    Vec2f_t dir = v2f_sub(end, start);
	Vec2f_t normal = (Vec2f_t){.x=-dir.y, .y=dir.x};

	if ((dStart < 0.0f) && (dEnd < 0.0f)) {return;}
	


	//Project into screen horizontally
	int startX = r_getCentreX(start, resolution);
	int endX = r_getCentreX(end, resolution);
	if (startX == endX) {return; /* Infinitely thin, don't draw. */}

	//Calculate depth
	float startDepth = MAX(v2f_dist(start, camera.position), NEAR_PLANE);
	float endDepth = MAX(v2f_dist(end, camera.position), NEAR_PLANE);

	//Find left/rightmost
	int leftMost, rightMost;
	float lInvDepth, rInvDepth;
	float leftT, rightT;
	if (startX < endX) {
		leftMost = startX; lInvDepth = 1.0f / startDepth;
		rightMost = endX;  rInvDepth = 1.0f / endDepth;
		leftT = startT; rightT = endT;
	} else {
		rightMost = startX; rInvDepth = 1.0f / startDepth;
		leftMost = endX;    lInvDepth = 1.0f / endDepth;
		leftT = endT; rightT = startT;
	}
	int range = rightMost - leftMost;

	int leftMostClamp = fmax(leftMost, 0);
	int rightMostClamp = fmin(rightMost, resolution.x);
	if ((rightMostClamp < 0) || (leftMostClamp >= resolution.x)) {return; /* Offscreen horizontally */}



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
	float textureX = 0.0f;
	for (int screenX=leftMostClamp; screenX<=rightMostClamp; screenX++) {
		float interp = (float)(screenX - leftMost) / (float)(range);
		float invDistance = MIN(f_lerp(lInvDepth, rInvDepth, interp), 1.0f / NEAR_PLANE);
		float depthF = 1.0f / invDistance;

		float t = f_lerp(leftT, rightT, interp);
		textureX = (int)(t * (float)(TEXTURE_RESOLUTION.x));

		if (isSolid) {
			r_drawSolidColumn(
				closeSectorID,
				screenX, aspectRatio*invDistance, (int)(textureX),
				fbPTR, resolution, thisLineDef->texture
			);
		} else {
			r_drawPortalColumn(
				closeSectorID, farSectorID,
				screenX, aspectRatio*invDistance, (int)(textureX),
				fbPTR, resolution, thisLineDef->texture
			);
		}
	}
}



float r_getLineDefDistance(const LineDef_t* thisLineDef) {
	//Get distance from camera.
	Vec2f_t start = g_vertices[thisLineDef->vStart];
	Vec2f_t end = g_vertices[thisLineDef->vEnd];

	Vec2f_t dir = v2f_sub(end, start);
	float dirLen = v2f_len(dir);
	dir = v2f_div(dir, dirLen); //Normalise
	Vec2f_t normal = (Vec2f_t){.x=-dir.y, .y=dir.x};

	Vec2f_t delta = v2f_sub(camera.position, start);
	float projD = v2f_dot(delta, dir);
	float projN = v2f_dot(delta, normal);

	if (projD < 0.0f) {return v2f_len(delta);}
	else if (projD > dirLen) {return v2f_dist(camera.position, end);}
	else {return fabsf(projN);}
}


int r_compareSorts(const void* a, const void* b) {
	float dA = ((const LineDefSort_t*)a)->distance;
	float dB = ((const LineDefSort_t*)b)->distance;

	if (dA < dB) {return -1;}
	if (dA > dB) {return  1;}
	return 0; //EQU.
}


void r_sortLineDefs(
	LineDef_t** result, unsigned int* numValidLineDefs
) {
	//Find LDs nearest to furthest.
	LineDefSort_t* sorts = calloc(g_numLineDefs, sizeof(LineDefSort_t));
	unsigned int numSorts = 0u;

	for (unsigned int ldIndex=0u; ldIndex<g_numLineDefs; ldIndex++) {
		LineDef_t* thisLineDef = g_lineDefs + ldIndex;
		if (!(thisLineDef->isValid)) {continue;}
		(*numValidLineDefs)++;
		sorts[numSorts++] = (LineDefSort_t){
			.distance=fabsf(r_getLineDefDistance(thisLineDef)),
			.lineDef=thisLineDef
		};
	}

	qsort(sorts, numSorts, sizeof(LineDefSort_t), r_compareSorts);

	for (unsigned int sortIndex=0u; sortIndex<numSorts; sortIndex++) {result[sortIndex] = sorts[sortIndex].lineDef;}

	free(sorts);
}


void r_drawFrame(const Vec2i_t resolution) {
	RGB_t* fbPTR = t_getFramebufferPTR();
	r_clearColumnBuffers(resolution); //Reset depth data & column bottom/top data for this frame.


	//Sort near-to-far.
	LineDef_t** sortedLineDefs = calloc(g_numLineDefs, sizeof(LineDef_t*));
	unsigned int numValidLineDefs = 0u;
	r_sortLineDefs(sortedLineDefs, &numValidLineDefs);


	for (unsigned int ldIndex=0u; ldIndex<numValidLineDefs; ldIndex++) {
		LineDef_t* thisLineDef = sortedLineDefs[ldIndex];
		r_drawLineDef(thisLineDef, resolution, fbPTR);
	}

	free(sortedLineDefs);
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


int r_loadTextures(const char** texturePaths, const unsigned int numTexturePaths) {
	unsigned int numValidTextures = 0u;
	for (unsigned int i=0u; i<numTexturePaths; i++) {
		const char* path = texturePaths[i];

		RGB_t* pixelData;
		if (!r_loadTexture(path, &pixelData)) {printf("Failed to load [%s]\n", path); return FALSE; /* Failed to load texture */}

		textures[i] = pixelData;
		printf("Loaded [%s] successfully.\n", path);
	}

	return TRUE; //Success
}
//////// INITIALISATION ////////


