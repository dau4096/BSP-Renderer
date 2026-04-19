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



#endif