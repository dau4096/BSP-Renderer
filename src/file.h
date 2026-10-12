/* loader.h */
#ifndef LOADER_H
#define LOADER_H


#include "types.h"


//////// XML FILE READING ////////
int f_loadGeo(const char* filePath);
//////// XML FILE READING ////////


//////// FILE WRITING ////////
int f_writeHTMLScreenshot(const char* filePath, const Vec2i_t resolution, const int uiHeight);

void f_getScreenshotFileName(char** filePath, unsigned int* length, const unsigned int frameNumber);
//////// FILE WRITING ////////


#endif
