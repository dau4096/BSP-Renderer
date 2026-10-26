/* menu.h */
#ifndef MENU_H
#define MENU_H

#include "../src/types.h"


//////// TYPES ////////
#define MENU_MAX_ENTRIES 64u
#define MENU_MAX_STRING_LENGTH 64u
#define MENU_DISPLAY_WIDTH 48u
#define MENU_VALUE_DISPLAY_LENGTH 16u


typedef enum {
	TYPE_NONE, TYPE_INT, TYPE_BOOL, TYPE_CHAR,
	TYPE_FLOAT, TYPE_STR, TYPE_UINT,
	TYPE_SEPERATOR, TYPE_CALLBACK,
	NUM_ENTRY_TYPES
} EntryType_e;

typedef struct {
	char* title; //Name of this entry.
	int isValid;
	void* data;
	int (*callback)(void); //Only used for TYPE_CALLBACK. NULL otherwise. Must return success/fail.
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

int mn_addIntEntry(Menu_t* thisMenu, char* title, const int defaultValue);
int mn_addBoolEntry(Menu_t* thisMenu, char* title, const int defaultValue);
int mn_addCharEntry(Menu_t* thisMenu, char* title, const char defaultValue);
int mn_addFloatEntry(Menu_t* thisMenu, char* title, const float defaultValue);
int mn_addStringEntry(Menu_t* thisMenu, char* title, const char* defaultValue);
int mn_addUIntEntry(Menu_t* thisMenu, char* title, const unsigned int defaultValue);
int mn_addSeperator(Menu_t* thisMenu, char* title);
int mn_addCallbackEntry(Menu_t* thisMenu, char* title, int (*func)(void));

int mn_deleteEntry(Menu_t* thisMenu, const char* title);
//////// MENU MANAGEMENT ////////



//////// MENU USAGE ////////
int mn_draw(const Menu_t* thisMenu, const Vec2i_t resolutionChars);
int mn_getEntry(const Menu_t* thisMenu, const char* entryName, MenuEntry_t** thisEntry);
int mn_editEntry(Menu_t* thisMenu);


//////// VALUE SETTING ////////
int mn_updateIntEntry(Menu_t* thisMenu, const char* title, int newValue);
int mn_updateBoolEntry(Menu_t* thisMenu, const char* title, int newValue);
int mn_updateCharEntry(Menu_t* thisMenu, const char* title, char newValue);
int mn_updateFloatEntry(Menu_t* thisMenu, const char* title, float newValue);
int mn_updateStringEntry(Menu_t* thisMenu, const char* title, char* newValue);
int mn_updateUIntEntry(Menu_t* thisMenu, const char* title, unsigned int newValue);
//////// VALUE SETTING ////////

//////// VALUE RETRIEVAL ////////
int mn_getInt(Menu_t* thisMenu, char* title, int* ptr);
int mn_getBool(Menu_t* thisMenu, char* title, int* ptr);
int mn_getChar(Menu_t* thisMenu, char* title, char* ptr);
int mn_getFloat(Menu_t* thisMenu, char* title, float* ptr);
int mn_getString(Menu_t* thisMenu, char* title, char** ptr);
int mn_getUInt(Menu_t* thisMenu, char* title, unsigned int* ptr);
//////// VALUE RETRIEVAL ////////

extern int isEditingEntry;
extern int finishedEditingEntry;
//////// MENU USAGE ////////



#endif