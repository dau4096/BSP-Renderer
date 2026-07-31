/* types.h */
#ifndef TYPES_H
#define TYPES_H


#include <stdint.h>


//"Boolean"
#define TRUE 1
#define FALSE 0


//Colours
typedef struct {uint8_t r, g, b;} RGB_t;
#define RGB_BLACK 	((RGB_t){  0u,   0u,   0u})
#define RGB_GREY 	((RGB_t){127u, 127u, 127u})
#define RGB_WHITE 	((RGB_t){255u, 255u, 255u})
#define RGB_RED 	((RGB_t){255u,   0u,   0u})
#define RGB_GREEN 	((RGB_t){  0u, 255u,   0u})
#define RGB_BLUE 	((RGB_t){  0u,   0u, 255u})
#define RGB_YELLOW 	((RGB_t){255u, 255u,   0u})
#define RGB_CYAN 	((RGB_t){  0u, 255u, 255u})
#define RGB_MAGENTA ((RGB_t){255u,   0u, 255u})


//Vectors
typedef struct {
	int x, y;
} Vec2i_t;

typedef struct {
	float x, y;
} Vec2f_t;



//Geometry
typedef struct {
	unsigned int vStart;      //Start vertex ID
	unsigned int vEnd;       //End vertex ID
	int frontSector;        //Sector ID this LineDef_t belongs to
	int backSector;        //-1 if solid wall, else index of neighbouring sector.
	unsigned int texture; //Texture to draw.
						 //
	int isValid;        //Was it created correctly?
} LineDef_t;


typedef struct {
	float floorHeight; unsigned int floorTexture; RGB_t floorColour; //Floor data
	float ceilingHeight; unsigned int ceilingTexture; RGB_t ceilingColour; //Ceiling data
	uint8_t flags; //Settings about this sector.
	unsigned int* lineDefs;    //Array of IDs to LineDef_t[] array (Like 3D model indices)
	unsigned int numLineDefs; //Length of ID array [^^].
	uint8_t lightLevel; //Brightness of the sector, 0-255
} Sector_t;



//Rendering only
typedef struct {
	int valid;
	RGB_t* frontData;
	RGB_t* backData;
	Vec2i_t resolutionPX; //Resolution (Pixels)
	Vec2i_t resolutionCHARS; //Resolution (Characters)
} Buffer_t;


typedef struct {
	float distance;
	LineDef_t* lineDef;
} LineDefSort_t;


typedef struct {
	unsigned int row;
	unsigned int xStart;
	unsigned int xEnd;

	const Sector_t* sector;
	int isFloor; //Is it using floor data or ceiling data from ^^ ptr.

	int active; //Is this span currently being used or not?
} PlaneSpan_t;




//Other
typedef struct {
	Vec2f_t position; float Z; //Position, 2D with seperate height (z).
	Vec2f_t forward;
	float Zvelocity; //Vertical speed.

	float yaw;  //Yaw in radians
	float FOV;  //FOV in radians
	float near; //Minimum view distance
	float far;  //Maximum view distance
} Camera_t;


#endif