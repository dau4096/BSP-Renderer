/* loader.h */
#ifndef FILE_H
#define FILE_H


#include "types.h"
#include "graphics.h" //For the geometry datasets


//////// XML FILE READING ////////
extern const char* textureNames[MAX_TEXTURES];
int f_loadGeo(const char* filePath);

#ifdef EDITOR
int f_saveGeo(const char* filePath);
#endif
//////// XML FILE READING ////////


//////// FILE WRITING ////////
#ifndef EDITOR
int f_writeHTMLScreenshot(const char* filePath, const Vec2i_t resolution, const int uiHeight);

void f_getScreenshotFileName(char** filePath, unsigned int* length, const unsigned int frameNumber);
#endif
//////// FILE WRITING ////////


#endif
