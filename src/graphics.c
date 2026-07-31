/* graphics.c */
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#include "graphics.h"

#ifdef DEBUG_DRAW_ORDER
	#include "io.h"
#endif

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


#define PLANE_UV_SCALE 2.0f
#define PLANE_UV_OFFSET (Vec2f_t){.x=0.0f, .y=0.0f}
//////// CONSTANTS ////////



//////// COLUMN DATA ////////
typedef uint8_t Depth_t;
Depth_t* depthMap; //1D depthmap.
unsigned int* lowYMap; //Lowest pixel of each column allowed to be drawn to
unsigned int* topYMap; //Highest pixel of each column allowed to be drawn to

unsigned int* lowYMapOld; //Same as above, old.
unsigned int* topYMapOld; //Same as above, old.
unsigned int* floorYMap; //Contains floor extent data
unsigned int* ceilYMap; //Contains ceiling extent data


void r_reallocColumnBuffers(void) {
	//If they exist, remove then remake.
	if (lowYMap) {free(lowYMap);}
	lowYMap = calloc(framebuffer.resolutionPX.x, sizeof(unsigned int));
	if (lowYMapOld) {free(lowYMapOld);}
	lowYMapOld = calloc(framebuffer.resolutionPX.x, sizeof(unsigned int));
	if (floorYMap) {free(floorYMap);}
	floorYMap = calloc(framebuffer.resolutionPX.x, sizeof(unsigned int));

	if (depthMap) {free(depthMap);}
	depthMap = calloc(framebuffer.resolutionPX.x, sizeof(Depth_t));

	if (topYMap) {free(topYMap);}
	topYMap = calloc(framebuffer.resolutionPX.x, sizeof(unsigned int));
	if (topYMapOld) {free(topYMapOld);}
	topYMapOld = calloc(framebuffer.resolutionPX.x, sizeof(unsigned int));
	if (ceilYMap) {free(ceilYMap);}
	ceilYMap = calloc(framebuffer.resolutionPX.x, sizeof(unsigned int));
}


void r_clearColumnBuffers() {
	memset(lowYMap, (unsigned int)(0x00u), framebuffer.resolutionPX.x * sizeof(unsigned int)); //Reset to all 0x00 (0px, bottom of the screen) values.
	memset(lowYMapOld, (unsigned int)(0x00u), framebuffer.resolutionPX.x * sizeof(unsigned int)); //Reset to all 0x00 (0px, bottom of the screen) values.
	memset(depthMap, (Depth_t)(0xFFu), framebuffer.resolutionPX.x * sizeof(Depth_t)); //Reset to all 0xFF (255, max depth) values.
	for (unsigned int index=0u; index<framebuffer.resolutionPX.x; index++) {
		//Set to all [resY] values.
		topYMap[index] = framebuffer.resolutionPX.y;
		topYMapOld[index] = framebuffer.resolutionPX.y;
	}
}


Depth_t r_mapDepth(float depthF) {
	float t = log(depthF / camera.near) / log(camera.far / camera.near);
	return (Depth_t)(
		CLAMP(t * 255.0f, 0.0f, 255.0f) //Remap to 0-255.
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
uint8_t colourMap[256][256];
unsigned int fallbackTextureIndex;


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
int r_getCentreX(const Vec2f_t position) {
	Vec2f_t direction = v2f_sub(position, camera.position);

	float theta = atan2(direction.x, direction.y);
	float angleDelta = theta - camera.yaw;

	while (angleDelta >  M_PI) {angleDelta -= 2.0f * M_PI;}
	while (angleDelta < -M_PI) {angleDelta += 2.0f * M_PI;}

	float centreX = ((float)(framebuffer.resolutionPX.x) / 2.0f) * ((angleDelta * 2.0f / camera.FOV) + 1.0f);
	return (int)(centreX);
}


void r_getLineDefSectorProjections(
	const Sector_t* thisSector, float invDistance, int* lowY, int* topY
) {
	float projectedYFloor = (camera.Z - thisSector->floorHeight) * invDistance;
	float projectedYCeiling = (camera.Z - thisSector->ceilingHeight) * invDistance;

	*lowY = (int)((float)(framebuffer.resolutionPX.y) * (0.5f - projectedYFloor));
	*topY = (int)((float)(framebuffer.resolutionPX.y) * (0.5f - projectedYCeiling));
}



float r_inverseDistanceProjections(
	const Sector_t* thisSector, float aspectRatio,
	int yLow, int yTop,
	float* ceilDistance, float* floorDistance
) {
	//Inverses r_getLineDefSectorProjections for top/bottom of screen.
	float projectedYCeiling = 0.5f - (float)yTop / framebuffer.resolutionPX.y;
	float projectedYFloor = 0.5f - (float)yLow / framebuffer.resolutionPX.y;

	//proj = (deltaZ * aspectRatio) / distance
	//:. distance = (deltaZ * aspectRatio) / proj

	*ceilDistance = ((camera.Z - thisSector->ceilingHeight) * aspectRatio) / projectedYCeiling;
	*floorDistance = ((camera.Z - thisSector->floorHeight) * aspectRatio) / projectedYFloor;
}




RGB_t rgb_fetch(const RGB_t textureValue, const uint8_t lightLevel) {
	//Fetches pre-lit 8b values for each channel.
	return (RGB_t){
		.r=colourMap[lightLevel][textureValue.r],
		.g=colourMap[lightLevel][textureValue.g],
		.b=colourMap[lightLevel][textureValue.b]
	};
}



void r_drawSolidColumn(
	const int sectorID, int screenX, float invDistance, int textureX,
	RGB_t* fbPTR, unsigned int textureID, Vec2f_t interpPosition, float aspectRatio
) {
	float floatDepth = 1.0f / invDistance;
	Depth_t mappedDepth = r_mapDepth(floatDepth);
	if (depthMap[screenX] <= mappedDepth) {return; /* Occluded */}

	//Check column maps;
	int minYBound = lowYMap[screenX];
	int maxYBound = topYMap[screenX];
	if (minYBound >= maxYBound) {return; /* Column is full */}

	//Draw this wall collumn.
	int lowYBound, topYBound; //Area this column should span (inside segment)
	const Sector_t* thisSector = g_sectors + sectorID;
	r_getLineDefSectorProjections(
		thisSector, invDistance, &lowYBound, &topYBound
	);
	if ((topYBound<=minYBound) || (lowYBound>=maxYBound)) {return; /* Completely offscreen vertically. */}

	int yLow = fmax(lowYBound, minYBound);
	int yTop = fmin(topYBound, maxYBound);

	//Column is taken, column was solid.
	lowYMap[screenX] = 0;
	topYMap[screenX] = 0;

	floorYMap[screenX] = yLow;
	ceilYMap[screenX] = yTop;



#ifdef DEBUG_BORDERS
	//Draw floor border.
	*(fbPTR + screenX + (framebuffer.resolutionPX.x * yLow)) = RGB_RED;

	//Draw floor border.
	*(fbPTR + screenX + (framebuffer.resolutionPX.x * yTop)) = RGB_RED;

#else

	//Draws top-to-bottom vertically. (Image is flipped when drawing to console)
	//Draw wall.
	RGB_t* ptr = fbPTR + screenX + (framebuffer.resolutionPX.x * yLow);
	RGB_t* texPTR;
	if (!r_getColumn(textureID, textureX, &texPTR)) {return;}
	for (int y=yLow; y<yTop; y++) {
		float t = (float)(y - lowYBound) / (float)(topYBound - lowYBound);
		*ptr = rgb_fetch(*(texPTR + (int)(t * (float)(TEXTURE_RESOLUTION.y))), thisSector->lightLevel);
		ptr += framebuffer.resolutionPX.x;
	}
	depthMap[screenX] = mappedDepth;
#endif
}


void r_drawPortalColumn(
	const int closeSectorID, const int farSectorID,
	int screenX, float invDistance, int textureX,
	RGB_t* fbPTR, unsigned int textureID,
	Vec2f_t interpPosition, float aspectRatio
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
		nearSector, invDistance, &lowYBoundNearUnclamp, &topYBoundNearUnclamp
	);
	if ((topYBoundNearUnclamp<minYBound) || (lowYBoundNearUnclamp>=maxYBound)) {return; /* Completely offscreen vertically. */}
	int lowYBoundNear = CLAMP(lowYBoundNearUnclamp, minYBound, maxYBound);
	int topYBoundNear = CLAMP(topYBoundNearUnclamp, minYBound, maxYBound);

	//Far sector's projections;
	int lowYBoundFarUnclamp, topYBoundFarUnclamp;
	const Sector_t* farSector = g_sectors + farSectorID;
	r_getLineDefSectorProjections(
		farSector, invDistance, &lowYBoundFarUnclamp, &topYBoundFarUnclamp
	);
	int lowYBoundFar = CLAMP(lowYBoundFarUnclamp, minYBound, maxYBound);
	int topYBoundFar = CLAMP(topYBoundFarUnclamp, minYBound, maxYBound);


	int yLow = fmin(lowYBoundNear, lowYBoundFar);
	int yTop = fmax(topYBoundNear, topYBoundFar);



#ifdef DEBUG_BORDERS
	//Draw lower border
	if (lowYBoundNear < lowYBoundFar) {
		//Draw a connecting wall between them and fill Y fill data.
		lowYMap[screenX] = lowYBoundFar;
		*(fbPTR + screenX + (framebuffer.resolutionPX.x * lowYBoundFar)) = RGB_MAGENTA;
		*(fbPTR + screenX + (framebuffer.resolutionPX.x * lowYBoundNear)) = RGB_BLUE;
	} else {
		//Just fill Y fill data.
		lowYMap[screenX] = lowYBoundNear;
		*(fbPTR + screenX + (framebuffer.resolutionPX.x * lowYBoundNear)) = RGB_MAGENTA;
		*(fbPTR + screenX + (framebuffer.resolutionPX.x * lowYBoundFar)) = RGB_BLUE;
	}

	//Draw upper border.
	if (topYBoundNear > topYBoundFar) {
		//Draw a connecting wall between them and fill Y fill data.
		topYMap[screenX] = topYBoundFar;
		*(fbPTR + screenX + (framebuffer.resolutionPX.x * topYBoundFar)) = RGB_MAGENTA;
		*(fbPTR + screenX + (framebuffer.resolutionPX.x * topYBoundNear)) = RGB_BLUE;
	} else {
		//Just fill Y fill data.
		topYMap[screenX] = topYBoundNear;
		*(fbPTR + screenX + (framebuffer.resolutionPX.x * topYBoundNear)) = RGB_MAGENTA;
		*(fbPTR + screenX + (framebuffer.resolutionPX.x * topYBoundFar)) = RGB_BLUE;
	}


#else

	//Draws top-to-bottom vertically.
	RGB_t* ptr;
	RGB_t* texPTR;

	if (!r_getColumn(textureID, textureX, &texPTR)) {return;}
	if (lowYBoundNear <= lowYBoundFar) {
		//Draw a connecting wall between them and fill Y fill data.
		lowYMap[screenX] = lowYBoundFar;
		ptr = fbPTR + screenX + (framebuffer.resolutionPX.x * lowYBoundNear);
		for (int y=lowYBoundNear; y<lowYBoundFar; y++) {
			float t = (float)(y - lowYBoundNearUnclamp) / (float)(lowYBoundFarUnclamp - lowYBoundNearUnclamp);
			*ptr = rgb_umul(*(texPTR + (int)(t * (float)TEXTURE_RESOLUTION.y)), nearSector->lightLevel);
			ptr += framebuffer.resolutionPX.x;
		}
		floorYMap[screenX] = yLow;

	} else {
		//Just fill Y fill data. (Floor)
		lowYMap[screenX] = lowYBoundNear;
		floorYMap[screenX] = lowYBoundNear;
	}


	//Draw the upper (Y value, lower onscreen) section of the portal
	if (topYBoundNear >= topYBoundFar) {
		//Draw a connecting wall between them and fill Y fill data.
		topYMap[screenX] = topYBoundFar;
		ptr = fbPTR + screenX + (framebuffer.resolutionPX.x * topYBoundFar);
		for (int y=topYBoundFar; y<topYBoundNear; y++) {
			float t = (float)(y - topYBoundFarUnclamp) / (float)(topYBoundNearUnclamp - topYBoundFarUnclamp);
			*ptr = rgb_fetch(*(texPTR + (int)(t * (float)TEXTURE_RESOLUTION.y)), nearSector->lightLevel);
			ptr += framebuffer.resolutionPX.x;
		}
		ceilYMap[screenX] = yTop;

	} else {
		//Just fill Y fill data. (Ceiling)
		topYMap[screenX] = topYBoundNear;
		ceilYMap[screenX] = topYBoundNear;
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





void r_drawSpan(const PlaneSpan_t* thisSpan, RGB_t* fbPTR, const float aspectRatio) {
	//Draws horizontal span of floor/ceiling, textured.
	if (!thisSpan->active) {return; /* Invalid! */}

	unsigned int xStart = CLAMP(thisSpan->xStart, 0u, framebuffer.resolutionPX.x-1u);
	unsigned int xEnd = CLAMP(thisSpan->xEnd, 0u, framebuffer.resolutionPX.x-1u);
	if (thisSpan->row >= framebuffer.resolutionPX.y) {return;}


	const Sector_t* thisSector = thisSpan->sector;
	if (
		(thisSpan->isFloor && (thisSector->flags & 0x1)) || //If floor and floor textured
		(!thisSpan->isFloor && (thisSector->flags & 0x2)) //If ceiling and ceiling textured
	) {
		float tStart = (float)(xStart) / (float)(framebuffer.resolutionPX.x);
		float tEnd = (float)(xEnd) / (float)(framebuffer.resolutionPX.x);
		float HALF_FOV = camera.FOV/2.0f;
		float aStart = f_lerp(-HALF_FOV, HALF_FOV, tStart) + camera.yaw;
		float aEnd = f_lerp(-HALF_FOV, HALF_FOV, tEnd) + camera.yaw;


		float floorDistance; float ceilDistance;
		r_inverseDistanceProjections(
			thisSector, aspectRatio,
			thisSpan->row, thisSpan->row,
			&ceilDistance, &floorDistance
		);

		Vec2f_t startDelta = (Vec2f_t) {
			.x=sin(aStart), .y=cos(aStart)
		};
		Vec2f_t endDelta = (Vec2f_t) {
			.x=sin(aEnd), .y=cos(aEnd)
		};

		unsigned int spanTexture;
		if (thisSpan->isFloor) {
			//isFloor
			startDelta = v2f_mul(startDelta, floorDistance);
			endDelta = v2f_mul(endDelta, floorDistance);
			spanTexture = thisSector->floorTexture;

		} else {
			//isCeiling
			startDelta = v2f_mul(startDelta, ceilDistance);
			endDelta = v2f_mul(endDelta, ceilDistance);
			spanTexture = thisSector->ceilingTexture;
		}

		Vec2f_t startPosition = v2f_add(camera.position, startDelta);
		Vec2f_t endPosition = v2f_add(camera.position, endDelta);

		Vec2f_t startUV = v2f_add(v2f_div(startPosition, PLANE_UV_SCALE), PLANE_UV_OFFSET);
		Vec2f_t endUV = v2f_add(v2f_div(endPosition, PLANE_UV_SCALE), PLANE_UV_OFFSET);


		RGB_t* rowStartPtr = fbPTR + (thisSpan->row * framebuffer.resolutionPX.x);
		for (unsigned int screenX=xStart; screenX<=xEnd; screenX++) {
			float t = (float)(screenX - xStart) / (float)(xEnd - xStart);
			Vec2f_t interpUV = v2f_fract(v2f_lerp(startUV, endUV, t));
			Vec2i_t uvInt = (Vec2i_t){
				.x=(int)(fabsf(interpUV.x * TEXTURE_RESOLUTION.x)) % TEXTURE_RESOLUTION.x,
				.y=(int)(fabsf(interpUV.y * TEXTURE_RESOLUTION.y)) % TEXTURE_RESOLUTION.y
			};
			RGB_t* texColour = (textures[spanTexture] + (uvInt.x * TEXTURE_RESOLUTION.y)) + uvInt.y;
			*(rowStartPtr + screenX) = rgb_fetch(*texColour, thisSector->lightLevel);
		}

	} else {

		RGB_t thisColour = rgb_fetch(
			(thisSpan->isFloor) ? thisSector->floorColour : thisSector->ceilingColour,
			thisSector->lightLevel
		);
		RGB_t* rowStartPtr = fbPTR + (thisSpan->row * framebuffer.resolutionPX.x);
		for (unsigned int screenX=xStart; screenX<=xEnd; screenX++) {
			*(rowStartPtr + screenX) = thisColour;
		}

	}
}




void r_drawLineDef(const LineDef_t* thisLineDef, RGB_t* fbPTR) {
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
	int startX = r_getCentreX(start);
	int endX = r_getCentreX(end);
	if (startX == endX) {return; /* Infinitely thin, don't draw. */}

	//Calculate depth
	float startDepth = MAX(v2f_dist(start, camera.position), NEAR_PLANE);
	float endDepth = MAX(v2f_dist(end, camera.position), NEAR_PLANE);

	//Find left/rightmost
	int leftMost, rightMost;
	float lInvDepth, rInvDepth;
	Vec2f_t leftMostPosiiton, rightMostPosition;
	float leftT, rightT;
	if (startX < endX) {
		leftMost = startX; lInvDepth = 1.0f / startDepth;
		rightMost = endX;  rInvDepth = 1.0f / endDepth;
		leftT = startT; rightT = endT;
		leftMostPosiiton = start; rightMostPosition = end;
	} else {
		rightMost = startX; rInvDepth = 1.0f / startDepth;
		leftMost = endX;    lInvDepth = 1.0f / endDepth;
		leftT = endT; rightT = startT;
		leftMostPosiiton = end; rightMostPosition = start;
	}
	int range = rightMost - leftMost;

	int leftMostClamp = fmax(leftMost, 0);
	int rightMostClamp = fmin(rightMost, framebuffer.resolutionPX.x);
	if ((rightMostClamp < 0) || (leftMostClamp >= framebuffer.resolutionPX.x)) {return; /* Offscreen horizontally */}



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
	float aspectRatio = (float)(framebuffer.resolutionPX.x) / (float)(framebuffer.resolutionPX.y);
	float textureX = 0.0f;
	unsigned int lowestPossibleSpan = 0u;
	unsigned int highestPossibleSpan = framebuffer.resolutionPX.y;
	for (int screenX=leftMostClamp; screenX<=rightMostClamp; screenX++) {
		float interp = (float)(screenX - leftMost) / (float)(range);
		float invDistance = MIN(f_lerp(lInvDepth, rInvDepth, interp), 1.0f / NEAR_PLANE);
		float depthF = 1.0f / invDistance;
		Vec2f_t interpPosition = v2f_lerp(leftMostPosiiton, rightMostPosition, interp);


		//Copy old buffers from BEFORE drawing new frame's column
		//r_drawSolidColumn and r_drawPortalColumn modify these.
		lowYMapOld[screenX] = lowYMap[screenX];
		topYMapOld[screenX] = topYMap[screenX];
		lowestPossibleSpan = MIN(lowestPossibleSpan, lowYMap[screenX]);
		highestPossibleSpan = MAX(highestPossibleSpan, lowYMap[screenX]);
		floorYMap[screenX] = lowYMap[screenX];
		ceilYMap[screenX] = lowYMap[screenX];


		if (lowYMap[screenX] == topYMap[screenX]) {continue; /* Occluded */}


		float t = f_lerp(leftT, rightT, interp);
		textureX = (int)(t * (float)(TEXTURE_RESOLUTION.x));

		if (isSolid) {
			r_drawSolidColumn(
				closeSectorID,
				screenX, aspectRatio*invDistance, (int)(textureX),
				fbPTR, thisLineDef->texture, interpPosition, aspectRatio
			);
		} else {
			r_drawPortalColumn(
				closeSectorID, farSectorID,
				screenX, aspectRatio*invDistance, (int)(textureX),
				fbPTR, thisLineDef->texture, interpPosition, aspectRatio
			);
		}
	}



	//Find every floor span for every row.
	const Sector_t* thisSector = (g_sectors+closeSectorID);
	PlaneSpan_t currentSpan;
	currentSpan.active = FALSE;
	//Only checks in X and Y range that couldve been modified.
	for (unsigned int row=lowestPossibleSpan; row<highestPossibleSpan; row++) {

		for (unsigned int column=leftMostClamp; column<=rightMostClamp; column++) {
			//Only search the area acctually modified by the linedef drawing loop above
			int columnDidChange = !(
				(lowYMapOld[column] == lowYMap[column]) &&
				(topYMapOld[column] == topYMap[column])
			); //Column was never written to; was already filled.

			if (columnDidChange && (lowYMapOld[column] <= row) && (floorYMap[column] > row)) {
				if (currentSpan.active) {
					//Floor here. Modify current span for this row to include this column.
					currentSpan.xEnd = column;

				} else {
					//If no such span exists, create new one using sector floor's texture ID.
					currentSpan = (PlaneSpan_t){
						.row=row, .xStart=column, .xEnd=column,
						.sector=thisSector, .isFloor=TRUE,
						.active=TRUE
					};
				}


			} else if (columnDidChange && (topYMapOld[column] > row) && (ceilYMap[column] <= row)) {
				if (currentSpan.active) {
					//Ceiling here. Modify current span for this row to include this column.
					currentSpan.xEnd = column;

				} else {
					//If no such span exists, create new one using sector ceiling's texture ID.
					currentSpan = (PlaneSpan_t){
						.row=row, .xStart=column, .xEnd=column,
						.sector=thisSector, .isFloor=FALSE,
						.active=TRUE
					};
				}

			} else {
				//Current span must have ended.
				r_drawSpan(&currentSpan, fbPTR, aspectRatio); //Draw using it's extents and texture information.
				currentSpan.active = FALSE; //Invalidate span.
			}
		}

		if (currentSpan.active) {
			//Finish row by drawing current span.
			r_drawSpan(&currentSpan, fbPTR, aspectRatio); //Draw using it's extents and texture information.
			currentSpan.active = FALSE; //Invalidate span.
		}
	}
}



float r_getLineDefDistance(const LineDef_t* thisLineDef, const Vec2f_t position) {
	//Get distance from given position.
	Vec2f_t start = g_vertices[thisLineDef->vStart];
	Vec2f_t end = g_vertices[thisLineDef->vEnd];

	Vec2f_t dir = v2f_sub(end, start);
	float dirLen = v2f_len(dir);
	dir = v2f_div(dir, dirLen); //Normalise
	Vec2f_t normal = (Vec2f_t){.x=-dir.y, .y=dir.x};

	Vec2f_t delta = v2f_sub(position, start);
	float projD = v2f_dot(delta, dir);
	float projN = v2f_dot(delta, normal);

	if (projD < 0.0f) {return v2f_len(delta); /* Distance to [start] */}
	else if (projD > dirLen) {return v2f_dist(position, end); /* Distance to [end] */}
	else {return fabsf(projN); /* Distance of [projection] (Perpendicular to lineDefinition) */}
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
			.distance=fabsf(r_getLineDefDistance(thisLineDef, camera.position)),
			.lineDef=thisLineDef
		};
	}

	qsort(sorts, numSorts, sizeof(LineDefSort_t), r_compareSorts);

	for (unsigned int sortIndex=0u; sortIndex<numSorts; sortIndex++) {result[sortIndex] = sorts[sortIndex].lineDef;}

	free(sorts);
}



#ifdef DEBUG_DRAW_ORDER
int currentDrawNumber = 0u;
#endif

void r_drawFrame(void) {
	RGB_t* fbPTR = t_getFramebufferPTR();
	r_clearColumnBuffers(); //Reset depth data & column bottom/top data for this frame.


	//Sort near-to-far.
	LineDef_t** sortedLineDefs = calloc(g_numLineDefs, sizeof(LineDef_t*));
	unsigned int numValidLineDefs = 0u;
	r_sortLineDefs(sortedLineDefs, &numValidLineDefs);


#ifdef DEBUG_DRAW_ORDER
	if (keyMapPress[K_DEBUG_DRAW_INC]) {currentDrawNumber--;}
	if (keyMapPress[K_DEBUG_DRAW_DEC]) {currentDrawNumber++;}
	currentDrawNumber = CLAMP(currentDrawNumber, 0, numValidLineDefs);
	numValidLineDefs -= currentDrawNumber;
#endif


	for (unsigned int ldIndex=0u; ldIndex<numValidLineDefs; ldIndex++) {
		LineDef_t* thisLineDef = sortedLineDefs[ldIndex];
		r_drawLineDef(thisLineDef, fbPTR);
	}

	free(sortedLineDefs);
}
//////// DRAWING ////////







//////// INITIALISATION ////////
void r_initCamera(void) {
	camera = (Camera_t){
		.position=(Vec2f_t){.x=0.0f, .y=0.0f},
		.yaw=0.0f, .FOV=1.22173f, //70 degrees in radians
		.near=0.1f,
		.far=128.0f,
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


	//Create colourMap lookup values.
	for (unsigned int lightLevel=0u; lightLevel<=0xFFu; lightLevel++) {
		for (unsigned int channelValue=0u; channelValue<=0xFFu; channelValue++) {
			colourMap[lightLevel][channelValue] = (uint8_t)(
				(float)(lightLevel) * (float)(channelValue) / 255.0f
			);
		}
	}

	return TRUE; //Success
}
//////// INITIALISATION ////////


