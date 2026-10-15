/* graphics.c */
#include <stdio.h>


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
	RGB_t* fbPTR = t_getFramebufferPTR() + (start.y * framebuffer.resolutionPX.x);
	for (int x=MAX(0, MIN(start.x, end.x)); x<=MIN(framebuffer.resolutionPX.x-1, MAX(start.x, end.x)); x++) {
		*(fbPTR++) = colour;
	}
}

void r_drawLineVertical(const Vec2i_t start, const Vec2i_t end, const RGB_t colour) {
	//Draws a vertical line on the framebuffer.
	if (start.x != end.x) {return; /* Can't draw */}

	int mi = MAX(0, MIN(start.y, end.y));
	int ma = MIN(framebuffer.resolutionPX.y-1, MIN(start.y, end.y));

	RGB_t* fbPTR = t_getFramebufferPTR() + (mi * framebuffer.resolutionPX.x);
	for (int y=mi; y<=ma; y++) {
		*fbPTR = colour;
		fbPTR += framebuffer.resolutionPX.x;
	}
}

void r_drawLineBresenham(const Vec2i_t start, const Vec2i_t end, const RGB_t colour) {
	//Draws line on the framebuffer using a form of Bresenham's algorithm.
	int m = 2 * (end.y - start.y);
	int error = m - (end.x - start.x);
	for (int x=start.x, y=start.y; x<=end.x; x++) {
		t_writePX((Vec2i_t){.x=x, .y=y}, colour);

		error += m;
		if (error >= 0) { //Reached error limit, inc y.
			y++;
			error -= 2 * (end.x - start.x);
		}
	}
}

void r_drawLineAuto(const Vec2i_t start, const Vec2i_t end, const RGB_t colour) {
	if (start.x == end.x) {r_drawLineVertical(start, end, colour);}
	else if (start.y == end.y) {r_drawLineHorizontal(start, end, colour);}
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
	Vec2f_t geoDelta = v2f_sub(vertex, camera.position); //Delta from the camera (centre screen) to this vertex.
	float zoom = ZOOM_SCALING_FACTOR * camera.FOV; //Zoom value is stored in the camera's FOV value.
	Vec2f_t screenspaceDelta = (Vec2f_t){ //From 0 to 1.
		.x=(geoDelta.x/zoom) + 1.0f / 2.0f,
		.y=(geoDelta.y/zoom) + 1.0f / 2.0f
	};

	//We know screen resolution as framebuffer.resolutionPX
	*screenPosition = (Vec2i_t){ //Actual integer pixel position
		.x=screenspaceDelta.x * framebuffer.resolutionPX.x,
		.y=screenspaceDelta.y * framebuffer.resolutionPX.y
	};
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
	if (!(startOnscreen && endOnscreen)) {return; /* Offscreen. */}
	r_drawLineBresenham(startPX, endPX, RGB_RED);
	r_drawVertex(startPX, RGB_YELLOW);
	r_drawVertex(endPX, RGB_YELLOW);
}


void r_drawOrigin(void) {
	//Drawn as a vertex with overlaid cross.
	Vec2i_t screenPosition;
	if (!r_remapVertexToScreen((Vec2f_t){.x=0.0f, .y=0.0f}, &screenPosition)) {return; /* Offscreen */}
	

	r_drawVertex(screenPosition, RGB_WHITE);
	r_drawLineHorizontal(
		v2i_add(screenPosition, (Vec2i_t){.x=-5, .y=0}),
		v2i_add(screenPosition, (Vec2i_t){.x= 5, .y=0}),
		RGB_WHITE
	);
	r_drawLineVertical(
		v2i_add(screenPosition, (Vec2i_t){.x=0, .y=-5}),
		v2i_add(screenPosition, (Vec2i_t){.x=0, .y= 5}),
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
