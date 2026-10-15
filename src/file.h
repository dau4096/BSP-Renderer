/* loader.h */
#ifndef FILE_H
#define FILE_H


#include "types.h"


//////// XML FILE READING ////////
int f_loadGeo(const char* filePath);
//////// XML FILE READING ////////


//////// FILE WRITING ////////
#ifndef EDITOR
int f_writeHTMLScreenshot(const char* filePath, const Vec2i_t resolution, const int uiHeight);

void f_getScreenshotFileName(char** filePath, unsigned int* length, const unsigned int frameNumber);
#endif
//////// FILE WRITING ////////


#endif
