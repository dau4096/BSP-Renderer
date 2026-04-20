/* io.h */
#ifndef IO_H
#define IO_H


typedef enum {
	K_MOVE_FORE, K_MOVE_BACK, K_MOVE_LEFT, K_MOVE_RIGHT, K_MOVE_FAST,
	K_TURN_LEFT, K_TURN_RIGHT,
	K_QUIT,

	NUM_KEYS //Automatically gets "length" of valid key enums.
} KeyCode_e;
extern int keyMap[NUM_KEYS];



int io_init(void); //Returns success/failiure.
void io_quit(void);


void io_pollEvents(void);
void io_handleInputs(Camera_t* camera);



#endif