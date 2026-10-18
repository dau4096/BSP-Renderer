/* menu.c */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "../src/types.h"
#include "menu.h"



//////// MENU MANAGEMENT ////////
int mn_createNewMenu(char* title, Menu_t* thisMenu) {
	//Create new Menu_t instance.
	if (!thisMenu) {return FALSE; /* No ptr. */}

	thisMenu->isValid = TRUE;
	thisMenu->title = title;

	thisMenu->width = MENU_DISPLAY_WIDTH; //TEMPORARY
	thisMenu->numEntries = 0u;
	thisMenu->selectedEntry = MENU_MAX_ENTRIES;
	return TRUE; //Success
}


int mn_deleteMenu(Menu_t* thisMenu) {
	if (!thisMenu || !thisMenu->isValid) {return FALSE; /* Menu doesn't exist or is invalid. */}

	thisMenu->isValid = FALSE;
	thisMenu->title[0] = '\0';

	thisMenu->numEntries = 0u;
	thisMenu->selectedEntry = MENU_MAX_ENTRIES;
	return TRUE; //Success
}




int mn_addIntEntry(Menu_t* thisMenu, char* title, const int defaultValue) {
	if (!thisMenu || !thisMenu->isValid) {return FALSE; /* Menu doesn't exist or is invalid. */}
	if (thisMenu->numEntries == MENU_MAX_ENTRIES) {return FALSE; /* Can't add another. */}

	thisMenu->entries[thisMenu->numEntries] = (MenuEntry_t){
		.title=strdup(title), .isValid=TRUE,
		.data=(void*)(malloc(sizeof(int))), //Allocate space for this entry's value.
		.type=TYPE_INT
	};
	*(int*)(thisMenu->entries[thisMenu->numEntries].data) = defaultValue;
	thisMenu->numEntries++;

	return TRUE; //Success
}


int mn_addBoolEntry(Menu_t* thisMenu, char* title, const int defaultValue) {
	if (!thisMenu || !thisMenu->isValid) {return FALSE; /* Menu doesn't exist or is invalid. */}
	if (thisMenu->numEntries == MENU_MAX_ENTRIES) {return FALSE; /* Can't add another. */}

	thisMenu->entries[thisMenu->numEntries] = (MenuEntry_t){
		.title=strdup(title), .isValid=TRUE,
		.data=(void*)(malloc(sizeof(int))), //Allocate space for this entry's value.
		.type=TYPE_BOOL
	};
	*(int*)(thisMenu->entries[thisMenu->numEntries].data) = defaultValue;
	thisMenu->numEntries++;

	return TRUE; //Success
}


int mn_addCharEntry(Menu_t* thisMenu, char* title, const char defaultValue) {
	if (!thisMenu || !thisMenu->isValid) {return FALSE; /* Menu doesn't exist or is invalid. */}
	if (thisMenu->numEntries == MENU_MAX_ENTRIES) {return FALSE; /* Can't add another. */}

	thisMenu->entries[thisMenu->numEntries] = (MenuEntry_t){
		.title=strdup(title), .isValid=TRUE,
		.data=(void*)(malloc(sizeof(char))), //Allocate space for this entry's value.
		.type=TYPE_CHAR
	};
	*(char*)(thisMenu->entries[thisMenu->numEntries].data) = defaultValue;
	thisMenu->numEntries++;

	return TRUE; //Success
}


int mn_addFloatEntry(Menu_t* thisMenu, char* title, const float defaultValue) {
	if (!thisMenu || !thisMenu->isValid) {return FALSE; /* Menu doesn't exist or is invalid. */}
	if (thisMenu->numEntries == MENU_MAX_ENTRIES) {return FALSE; /* Can't add another. */}

	thisMenu->entries[thisMenu->numEntries] = (MenuEntry_t){
		.title=strdup(title), .isValid=TRUE,
		.data=(void*)(malloc(sizeof(float))), //Allocate space for this entry's value.
		.type=TYPE_FLOAT
	};
	*(float*)(thisMenu->entries[thisMenu->numEntries].data) = defaultValue;
	thisMenu->numEntries++;

	return TRUE; //Success
}


int mn_addStringEntry(Menu_t* thisMenu, char* title, const char* defaultValue) {
    if (!thisMenu || !thisMenu->isValid) {return FALSE;}
    if (thisMenu->numEntries >= MENU_MAX_ENTRIES) {return FALSE;}
    if (!defaultValue) {return FALSE;}

    char* data = malloc(MENU_MAX_STRING_LENGTH);
    if (!data) {return FALSE;}

    snprintf(data, MENU_MAX_STRING_LENGTH, "%s", defaultValue);
    thisMenu->entries[thisMenu->numEntries] = (MenuEntry_t){
        .title = title, .isValid = TRUE,
        .data = data, .type = TYPE_STR
    };

    thisMenu->numEntries++;
    return TRUE;
}


int mn_addUIntEntry(Menu_t* thisMenu, char* title, const unsigned int defaultValue) {
	if (!thisMenu || !thisMenu->isValid) {return FALSE; /* Menu doesn't exist or is invalid. */}
	if (thisMenu->numEntries == MENU_MAX_ENTRIES) {return FALSE; /* Can't add another. */}

	thisMenu->entries[thisMenu->numEntries] = (MenuEntry_t){
		.title=strdup(title), .isValid=TRUE,
		.data=(void*)(malloc(sizeof(unsigned int))), //Allocate space for this entry's value.
		.type=TYPE_UINT
	};
	*(unsigned int*)(thisMenu->entries[thisMenu->numEntries].data) = defaultValue;
	thisMenu->numEntries++;

	return TRUE; //Success
}


int mn_addSeperator(Menu_t* thisMenu, char* title) {
	//Like a title in the middle of a menu.
	if (!thisMenu || !thisMenu->isValid) {return FALSE; /* Menu doesn't exist or is invalid. */}
	if (thisMenu->numEntries == MENU_MAX_ENTRIES) {return FALSE; /* Can't add another. */}

	thisMenu->entries[thisMenu->numEntries] = (MenuEntry_t){
		.title=strdup(title), .isValid=TRUE,
		.data=NULL, //No need to allocate space.
		.type=TYPE_SEPERATOR
	};
	thisMenu->numEntries++;

	return TRUE; //Success
}



int mn_getEntryIndex(const Menu_t* thisMenu, const char* entryName, unsigned int* entryID) {
	if (!thisMenu || !thisMenu->isValid) {return FALSE; /* Menu doesn't exist or is invalid. */}
	if (thisMenu->numEntries == 0u) {return FALSE;}
	for (unsigned int i=0u; i<thisMenu->numEntries; i++) {
		const MenuEntry_t* currEntry = thisMenu->entries + i;
		if (!currEntry->isValid) {continue;}
		if (strcmp(currEntry->title, entryName) == 0) {
			*entryID = i;
			return TRUE; //Success
		}
	}
	return FALSE; //Could not find in the menu.
}


int mn_deleteEntry(Menu_t* thisMenu, const char* title) {
	if (!thisMenu || !thisMenu->isValid) {return FALSE; /* Menu doesn't exist or is invalid. */}
	
	unsigned int thisEntryID;
	if (!mn_getEntryIndex(thisMenu, title, &thisEntryID)) {return FALSE;}

	if (thisMenu->selectedEntry == thisEntryID) {
		thisMenu->selectedEntry = (thisMenu->selectedEntry > 0u) ? thisMenu->selectedEntry-- : MENU_MAX_ENTRIES;
	}

	for (unsigned int i=thisEntryID; i<thisMenu->numEntries-1u; i++) {
		MenuEntry_t* dest = thisMenu->entries + i;
		MenuEntry_t* src = thisMenu->entries + i + 1;
		memcpy(
			dest, //Destination
			src, //Source
			sizeof(MenuEntry_t) //Size
		);
	}

	//Overwrite the last menu entry; will either be duplicate or already empty.
	MenuEntry_t* last = thisMenu->entries + MENU_MAX_ENTRIES - 1u;
	last->isValid = FALSE;
	last->title[0] = '\0';
	free(last->data);
	last->type = TYPE_NONE;

	thisMenu->numEntries--;

	return TRUE; //Success
}
//////// MENU MANAGEMENT ////////




void printCentered(const char* text, unsigned int width, const char* paddingChar, int inclEdges) {
	unsigned int len = (unsigned int)(strlen(text));

	if (len >= width) {
		printf("%s", text);
		return;
	}

	//if (inclEdges) {width -= 2u;}
	unsigned int leftPadding = (width - len) / 2u;
	unsigned int rightPadding = width - len - leftPadding;
	if (inclEdges) {leftPadding--; rightPadding--;}

	for (unsigned int i=0u; i<leftPadding; i++) {printf("%s", paddingChar);}
	if (inclEdges) {printf("┫");}
	printf("%s", text);
	if (inclEdges) {printf("┣");}
	for (unsigned int i=0u; i<rightPadding; i++) {printf("%s", paddingChar);}
}



//////// MENU DRAWING ////////
#define GET_CURSOR_ANSI_ESCAPE(X, Y, BUF) (sprintf(BUF, "\033[%u;%uH", Y, X))

int mn_draw(const Menu_t* thisMenu, const Vec2i_t resolutionChars) {
	if (!thisMenu || !thisMenu->isValid) {return FALSE; /* Menu doesn't exist or is invalid. */}

	const unsigned int startRow = 1u;
	const unsigned int startColumn = resolutionChars.x - 1u - thisMenu->width; //Extra 2 (+1-2) for each side formatting.
	char resetCursor[16u];
	GET_CURSOR_ANSI_ESCAPE(startColumn, startRow, resetCursor);
	unsigned int currentRow = startRow + 1;

	//Draw top formatting bar
	printf("%s┏", resetCursor);
	for (unsigned int i=0u; i<thisMenu->width-2u; i++) {printf("━");}
	printf("┓");

	//Draw title
	GET_CURSOR_ANSI_ESCAPE(startColumn, currentRow++, resetCursor);
	printf("%s┃", resetCursor);
	printCentered(thisMenu->title, thisMenu->width-2u, " ", FALSE);
	printf("┃");

	//Draw each entry
	for (unsigned int i=0u; i<thisMenu->numEntries; i++) {
		const MenuEntry_t* thisEntry = thisMenu->entries + i;
		GET_CURSOR_ANSI_ESCAPE(startColumn, currentRow++, resetCursor);
		printf("%s", resetCursor);
		char* highlight = (thisMenu->selectedEntry == i) ? "\033[7m" : "";
		char* resetColour = "\033[0m";

		if (thisEntry->type == TYPE_SEPERATOR) {
			printf("%s┣%s", resetCursor, highlight);
			printCentered(thisEntry->title, thisMenu->width-2u, "━", strlen(thisEntry->title) > 0u);
			printf("%s┫", resetColour);
		} else {
			unsigned int titleWidth = thisMenu->width - 3u - MENU_VALUE_DISPLAY_LENGTH;
			char valueBuffer[MENU_VALUE_DISPLAY_LENGTH + 1u]; valueBuffer[MENU_VALUE_DISPLAY_LENGTH] = '\0';
			memset(valueBuffer, ' ', MENU_VALUE_DISPLAY_LENGTH);
			switch (thisEntry->type) {
				case TYPE_INT: {snprintf(valueBuffer, MENU_VALUE_DISPLAY_LENGTH+1, "%*d", MENU_VALUE_DISPLAY_LENGTH, *(int*)(thisEntry->data)); break;}
				case TYPE_BOOL: {snprintf(valueBuffer, MENU_VALUE_DISPLAY_LENGTH+1, (*(int*)(thisEntry->data)) ? "YES" : "NO"); break;}
				case TYPE_CHAR: {snprintf(valueBuffer, MENU_VALUE_DISPLAY_LENGTH+1, "%c", *(char*)(thisEntry->data)); break;}
				case TYPE_FLOAT: {snprintf(valueBuffer, MENU_VALUE_DISPLAY_LENGTH+1, "%*f", MENU_VALUE_DISPLAY_LENGTH, *(float*)(thisEntry->data)); break;}
				case TYPE_STR: {snprintf(valueBuffer, MENU_VALUE_DISPLAY_LENGTH+1, "%*s", MENU_VALUE_DISPLAY_LENGTH, (char*)(thisEntry->data)); break;}
				case TYPE_UINT: {snprintf(valueBuffer, MENU_VALUE_DISPLAY_LENGTH+1, "%*u", MENU_VALUE_DISPLAY_LENGTH, *(unsigned int*)(thisEntry->data)); break;}
				case TYPE_SEPERATOR: case TYPE_NONE: default: {break;}
			}
			printf("┃%s%-*s→%*s%s┃", highlight, titleWidth, thisEntry->title, MENU_VALUE_DISPLAY_LENGTH, valueBuffer, resetColour);
		}
	}


	//Draw bottom formatting bar
	GET_CURSOR_ANSI_ESCAPE(startColumn, currentRow++, resetCursor);
	printf("%s┗", resetCursor);
	for (unsigned int i=0u; i<thisMenu->width-2u; i++) {printf("━");}
	printf("┛");
	fflush(stdout);

	return TRUE; //Success
}


int mn_getEntry(const Menu_t* thisMenu, const char* entryName, MenuEntry_t* thisEntry) {
	if (!thisMenu || !thisMenu->isValid) {return FALSE; /* Menu doesn't exist or is invalid. */}
	
	unsigned int thisEntryID;
	if (!mn_getEntryIndex(thisMenu, entryName, &thisEntryID)) {return FALSE;}

	memcpy(thisEntry, thisMenu->entries+thisEntryID, sizeof(MenuEntry_t));
	return TRUE; //Success
}
//////// MENU DRAWING ////////
