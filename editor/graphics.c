/* graphics.c */
#include <stdio.h>
#include <stdlib.h>

#include "graphics.h"

#include "../src/types.h"
#include "../src/maths.h"
#include "terminal.h"



//////// DATA ////////
unsigned int fallbackTextureIndex;


Camera_t camera; //The view
Camera_t* r_camera = &camera; //Ptr to ^^


unsigned int g_numVertices;
unsigned int g_numLineDefs;
unsigned int g_numSectors;

Vec2f_t* g_vertices; //2D positions.
LineDef_t* g_lineDefs; //LineDefs connecting vertices.
Sector_t* g_sectors; //Sectors made of LineDefs.



#define ZOOM_SCALING_FACTOR 10.0f /* For every 1 "unit" of zoom (camera.fov), how many units in Geo? */
//////// DATA ////////



//////// DRAWING ////////
//////// LINES ////////
void r_drawLineHorizontal(const Vec2i_t start, const Vec2i_t end, const RGB_t colour) {	
	//Draws a horizontal line on the framebuffer.
	if (start.y != end.y) {return; /* Can't draw */}
	if (
		(MAX(start.x, end.x) < 0) ||
		(MIN(start.x, end.x) >= framebuffer.resolutionPX.x)
	) {return;}

	int mi = CLAMP(MIN(start.x, end.x), 0, framebuffer.resolutionPX.x - 1);
	int ma = CLAMP(MAX(start.x, end.x), 0, framebuffer.resolutionPX.x - 1);

	RGB_t* fbPTR = t_getFramebufferPTR() + (start.y * framebuffer.resolutionPX.x) + mi;
	for (int x=mi; x<=ma; x++) {
		*(fbPTR++) = colour;
	}
}

void r_drawLineVertical(const Vec2i_t start, const Vec2i_t end, const RGB_t colour) {
	//Draws a vertical line on the framebuffer.
	if (start.x != end.x) {return; /* Can't draw */}
	if (
		(MAX(start.y, end.y) < 0) ||
		(MIN(start.y, end.y) >= framebuffer.resolutionPX.y)
	) {return;}


	int mi = CLAMP(MIN(start.y, end.y), 0, framebuffer.resolutionPX.y - 1);
	int ma = CLAMP(MAX(start.y, end.y), 0, framebuffer.resolutionPX.y - 1);

	RGB_t* fbPTR = t_getFramebufferPTR() + (mi * framebuffer.resolutionPX.x) + start.x;
	for (int y=mi; y<=ma; y++) {
		*fbPTR = colour;
		fbPTR += framebuffer.resolutionPX.x;
	}
}

void r_drawLineBresenham(const Vec2i_t start, const Vec2i_t end, const RGB_t colour) {
	int x = start.x;
	int dx = abs(end.x - start.x);
	int sx = (start.x < end.x) ? 1 : -1;

	int y = start.y;
	int dy = abs(end.y - start.y);
	int sy = (start.y < end.y) ? 1 : -1;

	int error = dx - dy;
	do {
		t_writePX((Vec2i_t){.x=x, .y=y}, colour);

		if ((x == end.x) && (y == end.y)) {break;}

		int e2 = 2 * error;
		if (e2 > -dy) {
			error -= dy;
			x += sx;
		}

		if (e2 < dx) {
			error += dx;
			y += sy;
		}
	} while (TRUE);
}

void r_drawLineAuto(const Vec2i_t start, const Vec2i_t end, const RGB_t colour) {
	if (start.y == end.y) {r_drawLineHorizontal(start, end, colour);}
	else if (start.x == end.x) {r_drawLineVertical(start, end, colour);}
	else {r_drawLineBresenham(start, end, colour);}
}
//////// LINES ////////

void r_drawVertex(const Vec2i_t screenPosition, const RGB_t colour) {
	//Draws vertex indicator at a given screen position.
	t_writePX(screenPosition, colour);
}
//////// DRAWING ////////



//////// GRAPHICS ////////
int r_remapVertexToScreen(const Vec2f_t vertex, Vec2i_t* screenPosition) {
	//Remaps vertex position to screen position.
	Vec2f_t geoDelta = v2f_sub(vertex, camera.position);

	float viewWidth = camera.FOV;
	float aspect = (float)(framebuffer.resolutionPX.x) / (float)(framebuffer.resolutionPX.y);
	float viewHeight = viewWidth / aspect;

	Vec2f_t screenspaceDelta = {
		.x = (geoDelta.x / viewWidth) + 0.5f,
		.y = (geoDelta.y / viewHeight) + 0.5f
	};

	screenPosition->x = (int)roundf(screenspaceDelta.x * framebuffer.resolutionPX.x);

	screenPosition->y = (int)roundf(screenspaceDelta.y * framebuffer.resolutionPX.y);

	return !(
		(screenPosition->x < 0) || (screenPosition->x >= framebuffer.resolutionPX.x) ||
		(screenPosition->y < 0) || (screenPosition->y >= framebuffer.resolutionPX.y)
	);
}






void r_drawLineDef(const LineDef_t* thisLineDef) {
	//Draws line representing a linedef.
	Vec2i_t startPX, endPX;
	int startOnscreen = r_remapVertexToScreen(g_vertices[thisLineDef->vStart], &startPX);
	int endOnscreen = r_remapVertexToScreen(g_vertices[thisLineDef->vEnd], &endPX);
	if (!(startOnscreen || endOnscreen)) {return; /* Offscreen. */}
	r_drawLineAuto(startPX, endPX, (thisLineDef->backSector == -1) ? RGB_RED : RGB_GREEN);
	r_drawVertex(startPX, RGB_YELLOW);
	r_drawVertex(endPX, RGB_YELLOW);
}


#define ORIGIN_SIZE 2
void r_drawOrigin(void) {
	//Drawn as a vertex with overlaid cross.
	Vec2i_t screenPosition;
	if (!r_remapVertexToScreen((Vec2f_t){.x=0.0f, .y=0.0f}, &screenPosition)) {return; /* Offscreen */}
	

	r_drawVertex(screenPosition, RGB_WHITE);
	r_drawLineHorizontal(
		v2i_add(screenPosition, (Vec2i_t){.x=-ORIGIN_SIZE, .y=0}),
		v2i_add(screenPosition, (Vec2i_t){.x= ORIGIN_SIZE, .y=0}),
		RGB_WHITE
	);
	r_drawLineVertical(
		v2i_add(screenPosition, (Vec2i_t){.x=0, .y=-ORIGIN_SIZE}),
		v2i_add(screenPosition, (Vec2i_t){.x=0, .y= ORIGIN_SIZE}),
		RGB_WHITE
	);
}


void r_drawGeo(void) {
	//Draw all geometry onscreen.
	for (Vec2f_t* thisVertex=g_vertices; thisVertex<g_vertices+g_numVertices; thisVertex++) {
		Vec2i_t vSPos;
		if (!r_remapVertexToScreen(*thisVertex, &vSPos)) {continue; /* Offscreen */}
		r_drawVertex(vSPos, RGB_BLUE);
	}

	for (LineDef_t* thisLineDef=g_lineDefs; thisLineDef<g_lineDefs+g_numLineDefs; thisLineDef++) {
		if (!thisLineDef->isValid) {continue; /* Don't try to draw invalid linedefs. */}
		r_drawLineDef(thisLineDef);
	}

	//Draw origin.
	r_drawOrigin();
}
//////// GRAPHICS ////////
