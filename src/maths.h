/* maths.h */
#ifndef MATHS_H
#define MATHS_H


#include <math.h>
#include "types.h"



//////// GENERIC MATHS ////////


static float f_lerp(float a, float b, float t) {
	return a + (b-a)*t;
}

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))


//////// GENERIC MATHS ////////



//////// VECTOR 2D MATHS ////////

static Vec2i_t v2i_add(const Vec2i_t a, const Vec2i_t b) {
	return (Vec2i_t){
		.x = (a.x+b.x),
		.y = (a.y+b.y)
	};
}

static Vec2f_t v2f_add(const Vec2f_t a, const Vec2f_t b) {
	return (Vec2f_t){
		.x = (a.x+b.x),
		.y = (a.y+b.y)
	};
}



static Vec2i_t v2i_sub(const Vec2i_t a, const Vec2i_t b) {
	return (Vec2i_t){
		.x = (a.x-b.x),
		.y = (a.y-b.y)
	};
}

static Vec2f_t v2f_sub(const Vec2f_t a, const Vec2f_t b) {
	return (Vec2f_t){
		.x = (a.x-b.x),
		.y = (a.y-b.y)
	};
}



static Vec2i_t v2i_mul(const Vec2i_t a, const int s) {
	return (Vec2i_t){
		.x = (a.x*s),
		.y = (a.y*s)
	};
}

static Vec2f_t v2f_mul(const Vec2f_t a, const float s) {
	return (Vec2f_t){
		.x = (a.x*s),
		.y = (a.y*s)
	};
}



static Vec2i_t v2i_div(const Vec2i_t a, const int s) {
	if (s == 0) {return (Vec2i_t){.x=0, .y=0}; /* Invalid, DIV0. */}
	return (Vec2i_t){
		.x = (a.x/s),
		.y = (a.y/s)
	};
}

static Vec2f_t v2f_div(const Vec2f_t a, const float s) {
	if (s == 0) {return (Vec2f_t){.x=0.0f, .y=0.0f}; /* Invalid, DIV0. */}
	return (Vec2f_t){
		.x = (a.x/s),
		.y = (a.y/s)
	};
}



static int v2i_dot(const Vec2i_t a, const Vec2i_t b) {
	return (a.x*b.x) + (a.y*b.y);
}

static float v2f_dot(const Vec2f_t a, const Vec2f_t b) {
	return (a.x*b.x) + (a.y*b.y);
}



static int v2i_lenSQ(const Vec2i_t a) {
	return (a.x*a.x) + (a.y*a.y);
}

static float v2f_lenSQ(const Vec2f_t a) {
	return (a.x*a.x) + (a.y*a.y);
}



static int v2i_len(const Vec2i_t a) {
	return (int)sqrtf((float)v2i_lenSQ(a));
}

static float v2f_len(const Vec2f_t a) {
	return sqrtf(v2f_lenSQ(a));
}



static Vec2i_t v2i_normalise(const Vec2i_t a) {
	int mag = (int)sqrtf((float)v2i_lenSQ(a));
	return v2i_div(a, mag);
}

static Vec2f_t v2f_normalise(const Vec2f_t a) {
	float mag = sqrtf(v2f_lenSQ(a));
	return v2f_div(a, mag);
}

//////// VECTOR 2D MATHS ////////




//////// COLOUR MATHS ////////

static RGB_t rgb_add(const RGB_t a, const RGB_t b) {
	return (RGB_t){
		.r=a.r+b.r, .g=a.g+b.g, .b=a.b+b.b
	};
}

static RGB_t rgb_sub(const RGB_t a, const RGB_t b) {
	return (RGB_t){
		.r=a.r-b.r, .g=a.g-b.g, .b=a.b-b.b
	};
}

static RGB_t rgb_fmul(const RGB_t a, float s) {
	return (RGB_t){
		.r=(uint8_t)(s * (float)(a.r)),
		.g=(uint8_t)(s * (float)(a.g)),
		.b=(uint8_t)(s * (float)(a.b))
	};
}

static RGB_t rgb_umul(const RGB_t a, uint8_t s) {
	return rgb_fmul(a, (float)(s) / 255.0f);	
}

static void rgb_quantise(RGB_t* colour) {
#ifdef COLOUR_QUANTISATION //Only modifies if necessary.
	colour->r = (colour->r >> 4) << 4;
	colour->g = (colour->g >> 4) << 4;
	colour->b = (colour->b >> 4) << 4;
#endif
}

//////// COLOUR MATHS ////////



#endif