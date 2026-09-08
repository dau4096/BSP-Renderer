/* maths.h */
#ifndef MATHS_H
#define MATHS_H

#include "types.h"
#include "trigLUT.h"



//////// GENERIC MATHS ////////


static float f_lerp(float a, float b, float t) {
	return a + (b-a)*t;
}
static Vec2f_t v2f_lerp(Vec2f_t a, Vec2f_t b, float t) {
	return (Vec2f_t){
		.x=a.x + (b.x-a.x)*t,
		.y=a.y + (b.y-a.y)*t
	};
}

static float f_abs(const float v) {return (v > 0) ? -v : v;}
static float f_floor(const float v) {return (float)((int)(v));}
static float f_round(const float v) {return f_floor(v + 0.5f);}
static float f_ceil(const float v) {return f_floor(v) + 1.0f;}
static float f_mod(float x, float y) {
	if (y == 0.0f) {return 0.0f;}
	int n = (int)(x / y);
	return x - (float)(n * y);
}

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define CLAMP(a,mi,ma) (((a)>(mi)) ? (((a)<(ma)) ? (a) : (ma)) : (mi))

#define MUL_RADIANS 0.017453f
#define MUL_DEGREES 57.29577f
#define M_PI 3.141592f


static float f_sqrtf(float x) {
	if (x < 0.0f) {return 0.0f;}
	if (x == 0.0f) {return 0.0f;}
	float g = (x < 1.0f) ? 1.0f : x;

	float next;
	while (TRUE) {
		next = 0.5f * (g + x / g);
		if (next == g) {break;}
		g = next;
	}

	return g;
}


static float i_sin(int angle) {
	//Wrap to 360 deg
	int a = angle % 360;
	if (a < 0) {a += 360;}

	int quad = a / SINE_LUT_SIZE; //[0-3] quadrant
	int idx  = a % SINE_LUT_SIZE; //[0-89] angle


	int lut_index;
	int sign;
	switch (quad) {
		case 0: {lut_index = idx;        sign =  1; break;} //[0-90] deg
		case 1: {lut_index = 89 - idx;   sign =  1; break;} //[90-180] deg
		case 2: {lut_index = idx;        sign = -1; break;} //[180-270] deg
		case 3: {lut_index = 89 - idx;   sign = -1; break;} //[270-360] deg
	}

	return sign * sin_LUT[lut_index];
}
static float i_cos(int angle)   {return i_sin(angle + 90);}
static float f_sin(float angle) {return i_sin((int)(f_round(angle) * MUL_DEGREES));}
static float f_cos(float angle) {return i_cos((int)(f_round(angle) * MUL_DEGREES));}

static float f_atan2(const float y, const float x) {
	float absY = (y < 0.0f) ? -y : y;
	float absX = (x < 0.0f) ? -x : x;

	if ((absX == 0.0f) && (absY == 0.0f)) {return 0.0f;}

	int angle;
	if (absX > absY) {
		int idx = (int)((absY / absX) * (float)(1 << ATAN_LUT_BITS));
		if (idx >= ATAN_LUT_SIZE) {idx = ATAN_LUT_SIZE - 1;}
		angle = atan_LUT[idx];

	} else {
		int idx = (int)((absX / absY) * (float)(1 << ATAN_LUT_BITS));
		if (idx >= ATAN_LUT_SIZE) {idx = ATAN_LUT_SIZE - 1;}
		angle = 90 - atan_LUT[idx];
	}

	if ((x >= 0.0f) && (y >= 0.0f)) {return (float)(angle) * MUL_RADIANS;}
	else if ((x < 0.0f) && (y >= 0.0f)) {return (float)(180 - angle) * MUL_RADIANS;}
	else if ((x < 0.0f) && (y < 0.0f)) {return (float)(180 + angle) * MUL_RADIANS;}
	else {return (float)(360 - angle) * MUL_RADIANS;}
}

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
	return (int)f_sqrtf((float)v2i_lenSQ(a));
}

static float v2f_len(const Vec2f_t a) {
	return f_sqrtf(v2f_lenSQ(a));
}



static int v2i_dist(const Vec2i_t a, const Vec2i_t b) {
	return v2i_len(v2i_sub(a, b));
}

static float v2f_dist(const Vec2f_t a, const Vec2f_t b) {
	return v2f_len(v2f_sub(a, b));
}



static Vec2i_t v2i_normalise(const Vec2i_t a) {
	int mag = (int)f_sqrtf((float)v2i_lenSQ(a));
	return v2i_div(a, mag);
}

static Vec2f_t v2f_normalise(const Vec2f_t a) {
	float mag = f_sqrtf(v2f_lenSQ(a));
	return v2f_div(a, mag);
}



static Vec2i_t v2i_min(const Vec2i_t a, const Vec2i_t b) {
	return (Vec2i_t){
		.x=MIN(a.x, b.x),
		.y=MIN(a.x, b.x)
	};
}

static Vec2f_t v2f_min(const Vec2f_t a, const Vec2f_t b) {
	return (Vec2f_t){
		.x=MIN(a.x, b.x),
		.y=MIN(a.x, b.x)
	};
}



static Vec2i_t v2i_max(const Vec2i_t a, const Vec2i_t b) {
	return (Vec2i_t){
		.x=MAX(a.x, b.x),
		.y=MAX(a.x, b.x)
	};
}

static Vec2f_t v2f_max(const Vec2f_t a, const Vec2f_t b) {
	return (Vec2f_t){
		.x=MAX(a.x, b.x),
		.y=MAX(a.x, b.x)
	};
}

static Vec2f_t v2f_fract(const Vec2f_t a) {
	//Only the fractional portion.
	return v2f_sub(
		a, (Vec2f_t){
			.x=(float)((int)(a.x)),
			.y=(float)((int)(a.y))
		}
	);
}

//////// VECTOR 2D MATHS ////////




//////// COLOUR MATHS ////////

static void rgb_unpack(const RGB_t colour, uint8_t* R, uint8_t* G, uint8_t* B) {
	*R = (uint8_t)((colour >> 8u) & 0xF8u);
	*G = (uint8_t)((colour >> 3u) & 0xFCu);
	*B = (uint8_t)((colour << 3u) & 0xF8u);
}

static RGB_t rgb_pack(const uint8_t R, const uint8_t G, const uint8_t B) {
	return (
		(uint8_t)((R & 0xF8u) << 8u) |
		(uint8_t)((G & 0xFCu) << 3u) |
		(uint8_t)((B & 0xF8u) >> 3u)
	);
}

static RGB_t rgb_add(const RGB_t a, const RGB_t b) {
	uint16_t R = MIN(
		((a >> 11u) + (b >> 11u)) & 0x1Fu,
		31u
	);
	uint16_t G = MIN(
		((a >> 5u) + (b >> 5u))  & 0x3Fu,
		63u
	);
	uint16_t B = MIN(
		(a + b) & 0x1Fu,
		31u
	);


	return (RGB_t)((R << 11u) | (G << 5u) | B);
}


static RGB_t rgb_sub(const RGB_t a, const RGB_t b) {
	int R = (int)((a >> 11u) - (b >> 11u));
	int G = (int)(((a >> 5u) & 0x3Fu) - ((b >> 5u) & 0x3Fu));
	int B = (int)((a & 0x1Fu) - (b & 0x1Fu));

	if (R < 0) {R = 0;}
	if (G < 0) {G = 0;}
	if (B < 0) {B = 0;}

	return (RGB_t)(
		(((uint16_t)(R) & 0x1Fu) << 11u) |
		(((uint16_t)(G) & 0x3Fu) << 5u) |
		((uint16_t)(B) & 0x1Fu)
	);
}


static RGB_t rgb_fmul(const RGB_t a, const float s) {
	uint16_t R = (uint16_t)((float)((a >> 11u) & 0x1Fu) * s);
	uint16_t G = (uint16_t)((float)((a >> 5u)  & 0x3Fu) * s);
	uint16_t B = (uint16_t)((float)(a & 0x1Fu) * s);

	return (RGB_t)((R << 11u) | (G << 5u) | B);
}

static RGB_t rgb_umul(const RGB_t a, const uint8_t s) {
	uint16_t r = (((a >> 11u) & 0x1Fu) * s) / 255u;
	uint16_t g = (((a >> 5u)  & 0x3Fu) * s) / 255u;
	uint16_t b = ((a & 0x1Fu) * s) / 255u;

	return (RGB_t)((r << 11u) | (g << 5u) | b);
}


static void rgb_quantise(RGB_t* colour) {
/*Completely removed; no need to quantise in PRIZM.
//Preserved in the case that I want it later.

#ifdef COLOUR_QUANTISATION //Only modifies if necessary.
	colour->r = (colour->r >> 4) << 4;
	colour->g = (colour->g >> 4) << 4;
	colour->b = (colour->b >> 4) << 4;
#endif
*/
}

//////// COLOUR MATHS ////////



#endif