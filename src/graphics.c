/* graphics.c */
#include <string.h>
#include <fxcg/display.h>
#include <fxcg/heap.h>

#include "graphics.h"


#include "types.h"
#include "maths.h"
#include "sort.h"
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
#define DEPTH_SCALE_K 0.04f


#define PLANE_UV_SCALE 2.0f
#define PLANE_UV_OFFSET (Vec2f_t){.x=0.0f, .y=0.0f}
//////// CONSTANTS ////////



//////// COLUMN DATA ////////
typedef uint8_t Depth_t;
Depth_t depthMap[LCD_WIDTH_PX]; //1D depthmap.
unsigned int lowYMap[LCD_WIDTH_PX]; //Lowest pixel of each column allowed to be drawn to
unsigned int topYMap[LCD_WIDTH_PX]; //Highest pixel of each column allowed to be drawn to

unsigned int lowYMapOld[LCD_WIDTH_PX]; //Same as above, old.
unsigned int topYMapOld[LCD_WIDTH_PX]; //Same as above, old.
unsigned int floorYMap[LCD_WIDTH_PX]; //Contains floor extent data
unsigned int ceilYMap[LCD_WIDTH_PX]; //Contains ceiling extent data


void r_clearColumnBuffers() {
	memset(lowYMap, (unsigned int)(0x00u), LCD_WIDTH_PX * sizeof(unsigned int)); //Reset to all 0x00 (0px, bottom of the screen) values.
	memset(lowYMapOld, (unsigned int)(0x00u), LCD_WIDTH_PX * sizeof(unsigned int)); //Reset to all 0x00 (0px, bottom of the screen) values.
	memset(depthMap, (Depth_t)(0xFFu), LCD_WIDTH_PX * sizeof(Depth_t)); //Reset to all 0xFF (255, max depth) values.
	for (unsigned int index=0u; index<LCD_WIDTH_PX; index++) {
		//Set to all [resY] values.
		topYMap[index] = LCD_HEIGHT_PX;
		topYMapOld[index] = LCD_HEIGHT_PX;
	}
}


Depth_t r_mapDepth(float depthF) {
	float x = (depthF - camera.near) / (camera.far - camera.near);
	float t = x / (x + DEPTH_SCALE_K*(1.0f - x));
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





//////// DRAWING ////////
int r_getCentreX(const Vec2f_t position) {
	Vec2f_t direction = v2f_sub(position, camera.position);

	float theta = f_atan2(direction.x, direction.y);
	float angleDelta = theta - camera.yaw;

	while (angleDelta >  M_PI) {angleDelta -= 2.0f * M_PI;}
	while (angleDelta < -M_PI) {angleDelta += 2.0f * M_PI;}

	float centreX = ((float)(LCD_WIDTH_PX) / 2.0f) * ((angleDelta * 2.0f / camera.FOV) + 1.0f);
	return (int)(centreX);
}


void r_getLineDefSectorProjections(
	const Sector_t* thisSector, float invDistance, int* lowY, int* topY
) {
	float projectedYFloor = (camera.Z - thisSector->floorHeight) * invDistance;
	float projectedYCeiling = (camera.Z - thisSector->ceilingHeight) * invDistance;

	*lowY = (int)((float)(LCD_HEIGHT_PX) * (0.5f - projectedYFloor));
	*topY = (int)((float)(LCD_HEIGHT_PX) * (0.5f - projectedYCeiling));
}



void r_inverseDistanceProjections(
	const Sector_t* thisSector, float aspectRatio,
	int yLow, int yTop,
	float* ceilDistance, float* floorDistance
) {
	//Inverses r_getLineDefSectorProjections for top/bottom of screen.
	float projectedYCeiling = 0.5f - (float)yTop / LCD_HEIGHT_PX;
	float projectedYFloor = 0.5f - (float)yLow / LCD_HEIGHT_PX;

	//proj = (deltaZ * aspectRatio) / distance
	//:. distance = (deltaZ * aspectRatio) / proj

	*ceilDistance = ((camera.Z - thisSector->ceilingHeight) * aspectRatio) / projectedYCeiling;
	*floorDistance = ((camera.Z - thisSector->floorHeight) * aspectRatio) / projectedYFloor;
}




RGB_t rgb_fetch(const RGB_t textureValue, const uint8_t lightLevel) {
	return rgb_umul(textureValue, lightLevel);
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

	int yLow = MAX(lowYBound, minYBound);
	int yTop = MIN(topYBound, maxYBound);

	//Column is taken, column was solid.
	lowYMap[screenX] = 0;
	topYMap[screenX] = 0;

	floorYMap[screenX] = yLow;
	ceilYMap[screenX] = yTop;



#ifdef DEBUG_BORDERS
	//Draw floor border.
	*(fbPTR + screenX + (LCD_WIDTH_PX * yLow)) = RGB_RED;

	//Draw floor border.
	*(fbPTR + screenX + (LCD_WIDTH_PX * yTop)) = RGB_RED;

#else

	//Draws top-to-bottom vertically. (Image is flipped when drawing to console)
	//Draw wall.
	RGB_t* ptr = fbPTR + screenX + (LCD_WIDTH_PX * yLow);
	RGB_t colour = rgb_fetch(RGB_RED, thisSector->lightLevel);
	for (int y=yLow; y<yTop; y++) {
		//float t = (float)(y - lowYBound) / (float)(topYBound - lowYBound);
		*ptr = colour;//rgb_fetch(colour, thisSector->lightLevel);
		ptr += LCD_WIDTH_PX;
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


	int yLow = MIN(lowYBoundNear, lowYBoundFar);
	int yTop = MAX(topYBoundNear, topYBoundFar);



#ifdef DEBUG_BORDERS
	//Draw lower border
	if (lowYBoundNear < lowYBoundFar) {
		//Draw a connecting wall between them and fill Y fill data.
		lowYMap[screenX] = lowYBoundFar;
		*(fbPTR + screenX + (LCD_WIDTH_PX * lowYBoundFar)) = RGB_MAGENTA;
		*(fbPTR + screenX + (LCD_WIDTH_PX * lowYBoundNear)) = RGB_BLUE;
	} else {
		//Just fill Y fill data.
		lowYMap[screenX] = lowYBoundNear;
		*(fbPTR + screenX + (LCD_WIDTH_PX * lowYBoundNear)) = RGB_MAGENTA;
		*(fbPTR + screenX + (LCD_WIDTH_PX * lowYBoundFar)) = RGB_BLUE;
	}

	//Draw upper border.
	if (topYBoundNear > topYBoundFar) {
		//Draw a connecting wall between them and fill Y fill data.
		topYMap[screenX] = topYBoundFar;
		*(fbPTR + screenX + (LCD_WIDTH_PX * topYBoundFar)) = RGB_MAGENTA;
		*(fbPTR + screenX + (LCD_WIDTH_PX * topYBoundNear)) = RGB_BLUE;
	} else {
		//Just fill Y fill data.
		topYMap[screenX] = topYBoundNear;
		*(fbPTR + screenX + (LCD_WIDTH_PX * topYBoundNear)) = RGB_MAGENTA;
		*(fbPTR + screenX + (LCD_WIDTH_PX * topYBoundFar)) = RGB_BLUE;
	}


#else

	//Draws top-to-bottom vertically.
	RGB_t* ptr;
	RGB_t colour = rgb_fetch(RGB_BLUE, nearSector->lightLevel);

	if (lowYBoundNear <= lowYBoundFar) {
		//Draw a connecting wall between them and fill Y fill data.
		lowYMap[screenX] = lowYBoundFar;
		ptr = fbPTR + screenX + (LCD_WIDTH_PX * lowYBoundNear);
		for (int y=lowYBoundNear; y<lowYBoundFar; y++) {
			//float t = (float)(y - lowYBoundNearUnclamp) / (float)(lowYBoundFarUnclamp - lowYBoundNearUnclamp);
			*ptr = colour;//rgb_umul(colour, nearSector->lightLevel);
			ptr += LCD_WIDTH_PX;
		}
		floorYMap[screenX] = yLow;

	} else {
		//Just fill Y fill data. (Floor)
		lowYMap[screenX] = lowYBoundNear;
		floorYMap[screenX] = lowYBoundNear;
	}


	//Draw the upper (Y value, lower onscreen) section of the portal
	colour = rgb_fetch(RGB_GREEN, nearSector->lightLevel);
	if (topYBoundNear >= topYBoundFar) {
		//Draw a connecting wall between them and fill Y fill data.
		topYMap[screenX] = topYBoundFar;
		ptr = fbPTR + screenX + (LCD_WIDTH_PX * topYBoundFar);
		for (int y=topYBoundFar; y<topYBoundNear; y++) {
			//float t = (float)(y - topYBoundFarUnclamp) / (float)(topYBoundNearUnclamp - topYBoundFarUnclamp);
			*ptr = colour;//rgb_fetch(colour, nearSector->lightLevel);
			ptr += LCD_WIDTH_PX;
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
	Vec2f_t leftDirection = (Vec2f_t){.x=f_sin(leftAngle), .y=f_cos(leftAngle)}; //Points direction of leftside of screen.
	Vec2f_t leftNormal = (Vec2f_t){.x=leftDirection.y, .y=-leftDirection.x}; //Rotate CW to face into view. Positive values means RIGHT of leftmost edge.

	float rightAngle = camera.yaw + (camera.FOV * 0.5f);
	Vec2f_t rightDirection = (Vec2f_t){.x=f_sin(rightAngle), .y=f_cos(rightAngle)}; //Points direction of rightside of screen.
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
		float t = (-leftDotStart) / (f_abs(leftDotEnd) + f_abs(leftDotStart));
		*start = v2f_add(originalStart, v2f_mul(originalDelta, t));
		*startT = t;

	} else if (rightDotStart < 0.0f) {
		//Start is offscreen to the right (Cannot be both - Would be behind camera and so have exited by this point.)
		float t = (-rightDotStart) / (f_abs(rightDotEnd) + f_abs(rightDotStart));
		*start = v2f_add(originalStart, v2f_mul(originalDelta, t));
		*startT = t;

	}

	//Handle end clipping.
	if (leftDotEnd < 0.0f) {
		//End is offscreen to the left
		float t = (-leftDotEnd) / (f_abs(leftDotStart) + f_abs(leftDotEnd));
		*end = v2f_sub(originalEnd, v2f_mul(originalDelta, t));
		*endT = 1.0f - t;

	}
	if (rightDotEnd < 0.0f) {
		//End is offscreen to the right
		float t = (-rightDotEnd) / (f_abs(rightDotStart) + f_abs(rightDotEnd));
		*end = v2f_sub(originalEnd, v2f_mul(originalDelta, t));
		*endT = 1.0f - t;

	}

	return TRUE; //In the view.
}





void r_drawSpan(const PlaneSpan_t* thisSpan, RGB_t* fbPTR, const float aspectRatio) {
	//Draws horizontal span of floor/ceiling, textured.
	if (!thisSpan->active) {return; /* Invalid! */}

	unsigned int xStart = CLAMP(thisSpan->xStart, 0u, LCD_WIDTH_PX-1u);
	unsigned int xEnd = CLAMP(thisSpan->xEnd, 0u, LCD_WIDTH_PX-1u);
	if (thisSpan->row >= LCD_HEIGHT_PX) {return;}


	const Sector_t* thisSector = thisSpan->sector;
	if (
		FALSE && (
		(thisSpan->isFloor && (thisSector->flags & 0x1)) || //If floor and floor textured
		(!thisSpan->isFloor && (thisSector->flags & 0x2))) //If ceiling and ceiling textured
	) { /* //Temporarily removed.
		float tStart = (float)(xStart) / (float)(LCD_WIDTH_PX);
		float tEnd = (float)(xEnd) / (float)(LCD_WIDTH_PX);
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
			.x=f_sin(aStart), .y=f_cos(aStart)
		};
		Vec2f_t endDelta = (Vec2f_t) {
			.x=f_sin(aEnd), .y=f_cos(aEnd)
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


		RGB_t* rowStartPtr = fbPTR + (thisSpan->row * LCD_WIDTH_PX);
		for (unsigned int screenX=xStart; screenX<=xEnd; screenX++) {
			float t = (float)(screenX - xStart) / (float)(xEnd - xStart);
			Vec2f_t interpUV = v2f_fract(v2f_lerp(startUV, endUV, t));
			Vec2i_t uvInt = (Vec2i_t){
				.x=(int)(f_abs(interpUV.x * TEXTURE_RESOLUTION.x)) % TEXTURE_RESOLUTION.x,
				.y=(int)(f_abs(interpUV.y * TEXTURE_RESOLUTION.y)) % TEXTURE_RESOLUTION.y
			};
			RGB_t* texColour = (textures[spanTexture] + (uvInt.x * TEXTURE_RESOLUTION.y)) + uvInt.y;
			*(rowStartPtr + screenX) = rgb_fetch(*texColour, thisSector->lightLevel);
		} */

	} else {

		RGB_t thisColour = rgb_fetch(
			(thisSpan->isFloor) ? thisSector->floorColour : thisSector->ceilingColour,
			thisSector->lightLevel
		);
		RGB_t* rowStartPtr = fbPTR + (thisSpan->row * LCD_WIDTH_PX);
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

	int leftMostClamp = MAX(leftMost, 0);
	int rightMostClamp = MIN(rightMost, LCD_WIDTH_PX);
	if ((rightMostClamp < 0) || (leftMostClamp >= LCD_WIDTH_PX)) {return; /* Offscreen horizontally */}



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
	float aspectRatio = (float)(LCD_WIDTH_PX) / (float)(LCD_HEIGHT_PX);
	float textureX = 0.0f;
	unsigned int lowestPossibleSpan = 0u;
	unsigned int highestPossibleSpan = LCD_HEIGHT_PX;
	for (int screenX=leftMostClamp; screenX<=rightMostClamp; screenX++) {
		float interp = (float)(screenX - leftMost) / (float)(range);
		float invDistance = MIN(f_lerp(lInvDepth, rInvDepth, interp), 1.0f / NEAR_PLANE);
		//float depthF = 1.0f / invDistance;
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
		textureX = (int)(t);//(int)(t * (float)(TEXTURE_RESOLUTION.x));

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
					//If no such span exists, create new one uf_sing sector floor's texture ID.
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
					//If no such span exists, create new one uf_sing sector ceiling's texture ID.
					currentSpan = (PlaneSpan_t){
						.row=row, .xStart=column, .xEnd=column,
						.sector=thisSector, .isFloor=FALSE,
						.active=TRUE
					};
				}

			} else {
				//Current span must have ended.
				r_drawSpan(&currentSpan, fbPTR, aspectRatio); //Draw uf_sing it's extents and texture information.
				currentSpan.active = FALSE; //Invalidate span.
			}
		}

		if (currentSpan.active) {
			//Finish row by drawing current span.
			r_drawSpan(&currentSpan, fbPTR, aspectRatio); //Draw uf_sing it's extents and texture information.
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
	else {return f_abs(projN); /* Distance of [projection] (Perpendicular to lineDefinition) */}
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
	LineDefSort_t* sorts = sys_calloc(g_numLineDefs, sizeof(LineDefSort_t));
	unsigned int numSorts = 0u;

	for (unsigned int ldIndex=0u; ldIndex<g_numLineDefs; ldIndex++) {
		LineDef_t* thisLineDef = g_lineDefs + ldIndex;
		if (!(thisLineDef->isValid)) {continue;}
		(*numValidLineDefs)++;
		sorts[numSorts++] = (LineDefSort_t){
			.distance=f_abs(r_getLineDefDistance(thisLineDef, camera.position)),
			.lineDef=thisLineDef
		};
	}

	v_sort(sorts, numSorts, sizeof(LineDefSort_t), r_compareSorts);

	for (unsigned int sortIndex=0u; sortIndex<numSorts; sortIndex++) {result[sortIndex] = sorts[sortIndex].lineDef;}

	sys_free(sorts);
}



#ifdef DEBUG_DRAW_ORDER
int currentDrawNumber = 0u;
#endif

void r_drawFrame(void) {
	RGB_t* fbPTR = t_getFramebufferPTR();
	r_clearColumnBuffers(); //Reset depth data & column bottom/top data for this frame.


	//Sort near-to-far.
	LineDef_t** sortedLineDefs = sys_calloc(g_numLineDefs, sizeof(LineDef_t*));
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

	sys_free(sortedLineDefs);
}
//////// DRAWING ////////







//////// INITIALISATION ////////
void r_initCamera(void) {
	camera = (Camera_t){
		.position=(Vec2f_t){.x=0.0f, .y=0.0f},
		.yaw=0.0f, .FOV=1.22173f, //70 degrees in radians
		.near=0.1f, .far=128.0f,
		.forward=(Vec2f_t){.x=0.0f, .y=1.0f}
	};
}
//////// INITIALISATION ////////


