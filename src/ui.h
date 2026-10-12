/* ui.h */
#ifndef UI_H
#define UI_H


//////// INTERFACE ////////
extern unsigned int UI_HEIGHT;

void ui_drawInterface(const Vec2i_t resolution, const int uiHeight);

void ui_getHTML(const Vec2i_t resolution, const int uiHeight, char** buf, unsigned int* size);
//////// INTERFACE ////////


#endif
