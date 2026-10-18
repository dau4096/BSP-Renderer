/* menu.h */
#ifndef MENU_H
#define MENU_H

#include "../src/types.h"


//////// TYPES ////////
#define MENU_MAX_ENTRIES 24u
#define MENU_MAX_STRING_LENGTH 64u
#define MENU_DISPLAY_WIDTH 32u
#define MENU_VALUE_DISPLAY_LENGTH 16u


typedef enum {
	TYPE_NONE, TYPE_INT, TYPE_BOOL, TYPE_CHAR,
	TYPE_FLOAT, TYPE_STR, TYPE_UINT, TYPE_SEPERATOR,
	NUM_ENTRY_TYPES
} EntryType_e;

typedef struct {
	char* title; //Name of this entry.
	int isValid;
	void* data;
	EntryType_e type; //Type enum for this entry
} MenuEntry_t;

typedef struct {
	char* title; //Name of this menu
	int isValid;
	unsigned int width; //Taken from max of each entry's width.

	unsigned int numEntries;
	MenuEntry_t entries[MENU_MAX_ENTRIES];
	unsigned int selectedEntry; //Set to MENU_MAX_ENTRIES when none are selected.
} Menu_t;
//////// TYPES ////////



//////// MENU MANAGEMENT ////////
int mn_createNewMenu(char* title, Menu_t* thisMenu);
int mn_deleteMenu(Menu_t* thisMenu);

int mn_addIntEntry(Menu_t* thisMenu, char* title, const int defaultValu);
int mn_addBoolEntry(Menu_t* thisMenu, char* title, const int defaultValu);
int mn_addCharEntry(Menu_t* thisMenu, char* title, const char defaultValue);
int mn_addFloatEntry(Menu_t* thisMenu, char* title, const float defaultValue);
int mn_addStringEntry(Menu_t* thisMenu, char* title, const char* defaultValue);
int mn_addUIntEntry(Menu_t* thisMenu, char* title, const unsigned int defaultValue);
int mn_addSeperator(Menu_t* thisMenu, char* title);

int mn_deleteEntry(Menu_t* thisMenu, const char* title);
//////// MENU MANAGEMENT ////////



//////// MENU USAGE ////////
int mn_draw(const Menu_t* thisMenu, const Vec2i_t resolutionChars);
int mn_getEntry(const Menu_t* thisMenu, const char* entryName, MenuEntry_t* thisEntry);
//////// MENU USAGE ////////



#endif