/* menu.c */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "../src/types.h"
#include "../src/io.h"
#include "menu.h"



//////// MENU MANAGEMENT ////////
int mn_createNewMenu(char* title, Menu_t* thisMenu) {
	//Create new Menu_t instance.
	if (!thisMenu) {return FALSE; /* No ptr. */}

	thisMenu->isValid = TRUE;
	thisMenu->title = title;

	thisMenu->width = MENU_DISPLAY_WIDTH; //TEMPORARY
	thisMenu->numEntries = 0u;
	thisMenu->selectedEntry = 0u;
	return TRUE; //Success
}


int mn_deleteMenu(Menu_t* thisMenu) {
	if (!thisMenu || !thisMenu->isValid) {return FALSE; /* Menu doesn't exist or is invalid. */}

	thisMenu->isValid = FALSE;
	thisMenu->title[0] = '\0';

	thisMenu->numEntries = 0u;
	thisMenu->selectedEntry = 0u;
	return TRUE; //Success
}




int mn_addIntEntry(Menu_t* thisMenu, char* title, const int defaultValue) {
	if (!thisMenu || !thisMenu->isValid) {return FALSE; /* Menu doesn't exist or is invalid. */}
	if (thisMenu->numEntries == MENU_MAX_ENTRIES) {return FALSE; /* Can't add another. */}

	thisMenu->entries[thisMenu->numEntries] = (MenuEntry_t){
		.title=strdup(title), .isValid=TRUE,
		.data=(void*)(malloc(sizeof(int))), //Allocate space for this entry's value.
		.callback=NULL,
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
		.callback=NULL,
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
		.callback=NULL,
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
		.callback=NULL,
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
        .title=title, .isValid=TRUE,
        .data=data, .callback=NULL,
        .type=TYPE_STR
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
		.callback=NULL,
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
		.callback=NULL,
		.type=TYPE_SEPERATOR
	};
	thisMenu->numEntries++;

	return TRUE; //Success
}


int mn_addCallbackEntry(Menu_t* thisMenu, char* title, int (*func)(void)) {
	//Calls some callback func when pressed..
	if (!thisMenu || !thisMenu->isValid) {return FALSE; /* Menu doesn't exist or is invalid. */}
	if (thisMenu->numEntries == MENU_MAX_ENTRIES) {return FALSE; /* Can't add another. */}

	thisMenu->entries[thisMenu->numEntries] = (MenuEntry_t){
		.title=strdup(title), .isValid=TRUE,
		.data=NULL, //No need to allocate space.
		.callback=func, //Stores func to call. Takes no args, returns success [TRUE/FALSE]
		.type=TYPE_CALLBACK
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

	if (thisMenu->selectedEntry >= thisEntryID) {
		thisMenu->selectedEntry = (thisMenu->selectedEntry > 0u) ? thisMenu->selectedEntry-1u : MENU_MAX_ENTRIES;
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
	MenuEntry_t* last = thisMenu->entries + --thisMenu->numEntries;
	last->isValid = FALSE;
	if (last->title) {last->title[0] = '\0';}
	free(last->data);
	last->type = TYPE_NONE;

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
int isEditingEntry = FALSE;
char entryBuffer[MENU_MAX_STRING_LENGTH + 1u];
char* entryBufferPtr = entryBuffer;

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
		if (!thisEntry->isValid) {continue;}
		GET_CURSOR_ANSI_ESCAPE(startColumn, currentRow++, resetCursor);
		printf("%s", resetCursor);
		char* highlight = (thisMenu->selectedEntry == i) ? "\033[7m" : "";
		char* resetColour = "\033[0m";

		if (thisEntry->type == TYPE_SEPERATOR) {
			printf("%s┣%s", resetCursor, highlight);
			printCentered(thisEntry->title, thisMenu->width-2u, "━", strlen(thisEntry->title) > 0u);
			printf("%s┫", resetColour);
		} else if (thisEntry->type == TYPE_CALLBACK) {
			printf("%s┃%s", resetCursor, highlight);
			printCentered(thisEntry->title, thisMenu->width-2u, " ", FALSE);
			printf("%s┃", resetColour);
		} else {
			unsigned int titleWidth = thisMenu->width - 3u - MENU_VALUE_DISPLAY_LENGTH;
			char valueBuffer[MENU_VALUE_DISPLAY_LENGTH + 1u]; valueBuffer[MENU_VALUE_DISPLAY_LENGTH] = '\0';
			memset(valueBuffer, ' ', MENU_VALUE_DISPLAY_LENGTH);
			if (isEditingEntry && (thisMenu->selectedEntry == i)) {
				//Value should be the currently typed text instead.
				snprintf(valueBuffer, MENU_VALUE_DISPLAY_LENGTH+1, "%*s", MENU_VALUE_DISPLAY_LENGTH, entryBuffer);
			} else {
				switch (thisEntry->type) {
					case TYPE_INT: {snprintf(valueBuffer, MENU_VALUE_DISPLAY_LENGTH+1, "%*d", MENU_VALUE_DISPLAY_LENGTH, *(int*)(thisEntry->data)); break;}
					case TYPE_BOOL: {snprintf(valueBuffer, MENU_VALUE_DISPLAY_LENGTH+1, (*(int*)(thisEntry->data)) ? "YES" : "NO"); break;}
					case TYPE_CHAR: {snprintf(valueBuffer, MENU_VALUE_DISPLAY_LENGTH+1, "%c", *(char*)(thisEntry->data)); break;}
					case TYPE_FLOAT: {snprintf(valueBuffer, MENU_VALUE_DISPLAY_LENGTH+1, "%*f", MENU_VALUE_DISPLAY_LENGTH, *(float*)(thisEntry->data)); break;}
					case TYPE_STR: {snprintf(valueBuffer, MENU_VALUE_DISPLAY_LENGTH+1, "%*s", MENU_VALUE_DISPLAY_LENGTH, (char*)(thisEntry->data)); break;}
					case TYPE_UINT: {snprintf(valueBuffer, MENU_VALUE_DISPLAY_LENGTH+1, "%*u", MENU_VALUE_DISPLAY_LENGTH, *(unsigned int*)(thisEntry->data)); break;}
					case TYPE_SEPERATOR: case TYPE_NONE: default: {break;}
				}
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


int mn_getEntry(const Menu_t* thisMenu, const char* entryName, MenuEntry_t** thisEntry) {
	if (!thisMenu || !thisMenu->isValid) {return FALSE; /* Menu doesn't exist or is invalid. */}
	
	unsigned int thisEntryID;
	if (!mn_getEntryIndex(thisMenu, entryName, &thisEntryID)) {return FALSE;}

	*thisEntry = (MenuEntry_t*)(thisMenu->entries + thisEntryID);
	return TRUE; //Success
}



int finishedEditingEntry;
int mn_editEntry(Menu_t* thisMenu) {
	finishedEditingEntry = FALSE;
	if (!thisMenu || !thisMenu->isValid || (thisMenu->selectedEntry == MENU_MAX_ENTRIES)) {return FALSE;}

	MenuEntry_t* selectedEntry = thisMenu->entries + thisMenu->selectedEntry;
	if ((selectedEntry->type == TYPE_NONE) || (selectedEntry->type == TYPE_SEPERATOR)) {return TRUE; /* Can't edit, but nothing failed. */}

	if (!isEditingEntry) {
		if (lastPressedChar == '\n') {
			//Start editing a value

			switch (selectedEntry->type) {
				case TYPE_BOOL: {
					*(int*)(thisMenu->entries[thisMenu->selectedEntry].data) = (*(int*)(thisMenu->entries[thisMenu->selectedEntry].data)) == FALSE; //Very long line... Invert state.
					finishedEditingEntry = TRUE;
					isEditingEntry = FALSE;
					lastPressedChar = '\0';
					break;
				}
				case TYPE_CALLBACK: {
					if (!thisMenu->entries[thisMenu->selectedEntry].callback()) {/* Something failed, Handle? */}
					finishedEditingEntry = TRUE;
					lastPressedChar = '\0';
					isEditingEntry = FALSE;
					break;
				}
				default: {
					isEditingEntry = TRUE;
					entryBufferPtr = entryBuffer;
					*entryBufferPtr = '\0';
					break;
				}
			}
		}
	} else {
		//Already editing. Check for input and such.
		if (lastPressedChar == '\n') {
			//Finished editing.
			isEditingEntry = FALSE;
			switch (selectedEntry->type) { //Convert from string rep into [whatever this entry wants]
				case TYPE_INT: {
					*(entryBufferPtr++) = '\0';
					*(int*)(thisMenu->entries[thisMenu->selectedEntry].data) = atoi(entryBuffer);
					break;
				}
				case TYPE_BOOL: {*(int*)(thisMenu->entries[thisMenu->selectedEntry].data) = !(*(char*)(thisMenu->entries[thisMenu->selectedEntry].data)); break;} //Very long line... Invert state.
				case TYPE_CHAR: {*(char*)(thisMenu->entries[thisMenu->selectedEntry].data) = *entryBufferPtr; break;} //Set to be last pressed char (stored at buffer ptr)
				case TYPE_FLOAT: {
					*(entryBufferPtr++) = '\0';
					*(float*)(thisMenu->entries[thisMenu->selectedEntry].data) = atof(entryBuffer);
					break;
				}
				case TYPE_STR: {
					*(entryBufferPtr++) = '\0';
					strcpy(
						(char*)(thisMenu->entries[thisMenu->selectedEntry].data),
						entryBuffer
					);
					break;
				}
				case TYPE_UINT: {
					*(entryBufferPtr++) = '\0';
					*(int*)(thisMenu->entries[thisMenu->selectedEntry].data) = strtoul(entryBuffer, NULL, 10);
					break;
				}
				default: {break;}
			}
			finishedEditingEntry = TRUE;

		} else if (lastPressedChar == '\b') {
			//Backspace by one.
			*entryBufferPtr = '\0';
			if (entryBufferPtr > entryBuffer) {entryBufferPtr--; /* Don't underflow! */}

		} else if (keyMapPress[K_QUIT]) {
			//Exit out w/o saving changes.
			keyMapPress[K_QUIT] = FALSE; //Don't quit prgm!
			entryBufferPtr = entryBuffer;
			isEditingEntry = FALSE;
		} else if (lastPressedChar != '\0') {
		    //Append typed character if there is space.
		    if (entryBufferPtr < entryBuffer + MENU_MAX_STRING_LENGTH) {
		        *entryBufferPtr++ = lastPressedChar;
		        *entryBufferPtr = '\0';
		    }
		}
	}
}




//////// VALUE SETTING ////////
int mn_updateIntEntry(Menu_t* thisMenu, const char* title, int newValue) {
	if (!thisMenu || !thisMenu->isValid || !title) {return FALSE;}

	MenuEntry_t* thisEntry;
	if (!mn_getEntry(thisMenu, title, &thisEntry)) {return FALSE;}

	*(int*)(thisEntry->data) = newValue;
	return TRUE;
}
int mn_updateBoolEntry(Menu_t* thisMenu, const char* title, int newValue) {
	if (!thisMenu || !thisMenu->isValid || !title) {return FALSE;}

	MenuEntry_t* thisEntry;
	if (!mn_getEntry(thisMenu, title, &thisEntry)) {return FALSE;}

	*(int*)(thisEntry->data) = newValue != FALSE;
	return TRUE;
}
int mn_updateCharEntry(Menu_t* thisMenu, const char* title, char newValue) {
	if (!thisMenu || !thisMenu->isValid || !title) {return FALSE;}

	MenuEntry_t* thisEntry;
	if (!mn_getEntry(thisMenu, title, &thisEntry)) {return FALSE;}

	*(char*)(thisEntry->data) = newValue;
	return TRUE;
}
int mn_updateFloatEntry(Menu_t* thisMenu, const char* title, float newValue) {
	if (!thisMenu || !thisMenu->isValid || !title) {return FALSE;}

	MenuEntry_t* thisEntry;
	if (!mn_getEntry(thisMenu, title, &thisEntry)) {return FALSE;}

	*(float*)(thisEntry->data) = newValue;
	return TRUE;
}
int mn_updateStringEntry(Menu_t* thisMenu, const char* title, char* newValue) {
	if (!thisMenu || !thisMenu->isValid || !title) {return FALSE;}

	MenuEntry_t* thisEntry;
	if (!mn_getEntry(thisMenu, title, &thisEntry)) {return FALSE;}

	strcpy((char*)(thisEntry->data), newValue);
	return TRUE;
}
int mn_updateUIntEntry(Menu_t* thisMenu, const char* title, unsigned int newValue) {
	if (!thisMenu || !thisMenu->isValid || !title) {return FALSE;}

	MenuEntry_t* thisEntry;
	if (!mn_getEntry(thisMenu, title, &thisEntry)) {return FALSE;}

	*(unsigned int*)(thisEntry->data) = newValue;
	return TRUE;
}
//////// VALUE SETTING ////////




//////// VALUE RETRIEVAL ////////
int mn_getInt(Menu_t* thisMenu, char* title, int* ptr) {
	if (!thisMenu || !thisMenu->isValid || !title || !ptr) {return FALSE;}

	MenuEntry_t* thisEntry;
	if (!mn_getEntry(thisMenu, title, &thisEntry)) {return FALSE;}

	*ptr = *(int*)(thisEntry->data);
	return TRUE;
}
int mn_getBool(Menu_t* thisMenu, char* title, int* ptr) {
	if (!thisMenu || !thisMenu->isValid || !title || !ptr) {return FALSE;}

	MenuEntry_t* thisEntry;
	if (!mn_getEntry(thisMenu, title, &thisEntry)) {return FALSE;}

	*ptr = *(int*)(thisEntry->data) != FALSE;
	return TRUE;
}
int mn_getChar(Menu_t* thisMenu, char* title, char* ptr) {
	if (!thisMenu || !thisMenu->isValid || !title || !ptr) {return FALSE;}

	MenuEntry_t* thisEntry;
	if (!mn_getEntry(thisMenu, title, &thisEntry)) {return FALSE;}

	*ptr = *(char*)(thisEntry->data);
	return TRUE;
}
int mn_getFloat(Menu_t* thisMenu, char* title, float* ptr) {
	if (!thisMenu || !thisMenu->isValid || !title || !ptr) {return FALSE;}

	MenuEntry_t* thisEntry;
	if (!mn_getEntry(thisMenu, title, &thisEntry)) {return FALSE;}

	*ptr = *(float*)(thisEntry->data);
	return TRUE;
}
int mn_getString(Menu_t* thisMenu, char* title, char** ptr) {
	if (!thisMenu || !thisMenu->isValid || !title || !ptr) {return FALSE;}

	MenuEntry_t* thisEntry;
	if (!mn_getEntry(thisMenu, title, &thisEntry)) {return FALSE;}

	*ptr = malloc((MENU_MAX_STRING_LENGTH + 1u) * sizeof(char));
	strcpy(*ptr, (char*)(thisEntry->data));
	return TRUE;
}
int mn_getUInt(Menu_t* thisMenu, char* title, unsigned int* ptr) {
	if (!thisMenu || !thisMenu->isValid || !title || !ptr) {return FALSE;}

	MenuEntry_t* thisEntry;
	if (!mn_getEntry(thisMenu, title, &thisEntry)) {return FALSE;}

	*ptr = *(unsigned int*)(thisEntry->data);
	return TRUE;
}
//////// VALUE RETRIEVAL ////////

//////// MENU DRAWING ////////
