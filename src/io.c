/* io.c */
#include <fxcg/keyboard.h>
#include <string.h>

#include "types.h"
#include "io.h"



#define K_INVALID NUM_KEYS


int keyMapPress[NUM_KEYS];
int keyMapHold[NUM_KEYS];

int keyMapPrevious[NUM_KEYS];



KeyCode_e io_getID(const int key) {
	//Assigns PRIZM keys to internal key enums.
	switch (key) {
		case KEY_CTRL_EXE: {return K_QUIT;}

		case KEY_CHAR_8: {return K_MOVE_FORE;}  //NP8
		case KEY_CHAR_2: {return K_MOVE_BACK;}  //NP2
		case KEY_CHAR_4: {return K_MOVE_LEFT;}  //NP4
		case KEY_CHAR_6: {return K_MOVE_RIGHT;} //NP6
		
		//Temporarily removed due to blocking input making Jumping/Multi-key inputs unusable.
		//case KEY_CHAR_5: {return K_MOVE_JUMP;}    //Centre
		//case KEY_CHAR_MULT: {return K_MOVE_FAST;} //×
		//Will be readded if nonblocking input is added.

		case KEY_CTRL_LEFT: {return K_TURN_LEFT;}   //←
		case KEY_CTRL_RIGHT: {return K_TURN_RIGHT;} //→
	
	#ifdef DEBUG_DRAW_ORDER
		case KEY_CHAR_7: {return K_DEBUG_DRAW_DEC;} //NP7
		case KEY_CHAR_9: {return K_DEBUG_DRAW_INC;} //NP9
	#endif

		default: {return K_INVALID;}
	}
}



void io_pollEvents(int* RUN) {
	memcpy(keyMapPrevious, keyMapHold, sizeof(keyMapHold));
	memset(keyMapPress, 0x00u, sizeof(keyMapPress));
	memset(keyMapHold, 0x00u, sizeof(keyMapHold));

	//Using BLOCKING input. Stops PRIZM idle-calculating the same view repeatedly.
	int key;
	GetKey(&key);

	KeyCode_e namedAction = io_getID(key);
	if (namedAction != K_INVALID) {keyMapHold[namedAction] = TRUE;}
	if (namedAction == K_QUIT) {*RUN = FALSE;}


	for (unsigned int i=0u; i<NUM_KEYS; i++) {
		keyMapPress[i] = keyMapHold[i] && !keyMapPrevious[i];
	}
}




