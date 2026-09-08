/* io.c */
#include <fxcg/keyboard.h>

#include "types.h"
#include "io.h"



int keyMapPress[NUM_KEYS];
int keyMapHold[NUM_KEYS];

int keyMapPrevious[NUM_KEYS];




void io_pollEvents(int* RUN) {
	//TBA proper, just exit for now.
	//Use blocking input
	*RUN = FALSE;
}




