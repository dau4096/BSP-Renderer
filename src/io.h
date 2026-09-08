/* io.h */
#ifndef IO_H
#define IO_H


//////// DATA ////////
typedef enum {
	K_MOVE_FORE, K_MOVE_BACK, K_MOVE_LEFT, K_MOVE_RIGHT,
	K_TURN_LEFT, K_TURN_RIGHT,
	K_MOVE_FAST, K_MOVE_JUMP,
	K_QUIT,

#ifdef DEBUG_DRAW_ORDER
	K_DEBUG_DRAW_INC, K_DEBUG_DRAW_DEC,
#endif

	NUM_KEYS //Automatically gets "length" of valid key enums.
} KeyCode_e;
extern int keyMapHold[NUM_KEYS];
extern int keyMapPress[NUM_KEYS];
//////// DATA ////////


//////// TICK ////////
void io_pollEvents(int* RUN);
//////// TICK ////////



#endif