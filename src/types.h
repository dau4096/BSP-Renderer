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


//Other
typedef struct {
	int valid;
	RGB_t* data;
	Vec2i_t resolution;
} Buffer_t;

typedef struct {
	unsigned int vStart; //Start vertex ID
	unsigned int vEnd;  //End vertex ID
	int frontSector;   //Sector ID this LineDef_t belongs to
	int backSector;   //-1 if solid wall, else index of neighbouring sector.
	RGB_t colour;    //Colour to draw.
} LineDef_t;

typedef struct {
	float floorHeight;
	float ceilingHeight;
	unsigned int* lineDefs;    //Array of IDs to LineDef_t[] array (Like 3D model indices)
	unsigned int numLineDefs; //Length of ID array [^^].
} Sector_t;

typedef struct {
	Vec2f_t position;
	float yaw; //Yaw in radians
	float FOV; //FOV in radians
	float maxDistance; //Maximum view distance
} Camera_t;


#endif