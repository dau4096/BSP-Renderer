/* io.c */
#include <stdio.h>
#include <stdlib.h>

#include "types.h"
#include "io.h"



int keyMap[NUM_KEYS];


#ifndef _WIN32 //Linux only;

#include <linux/input.h>
#include <linux/input-event-codes.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <termios.h>

int fd; //Input path

static struct termios oldTermios;

void io_init(void) {
	
	fd = open("/dev/input/event9", O_RDONLY | O_NONBLOCK);
	if (fd < 0) {
		perror("open");
		exit(1);
	}

	//Disable terminal input processing
	struct termios raw;
	tcgetattr(STDIN_FILENO, &oldTermios);

	raw = oldTermios;
	raw.c_lflag &= ~(ICANON | ECHO);  //Disable ln buf & echo

	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}


void io_quit(void) {
	//Restore terminal input
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &oldTermios);
}



int getID(int key) {
	//Get the ID from the key enum.
	switch(key) {
		case KEY_ESC: {return K_QUIT;}

		case KEY_W: {return K_MOVE_FORE;}
		case KEY_S: {return K_MOVE_BACK;}
		case KEY_A: {return K_MOVE_LEFT;}
		case KEY_D: {return K_MOVE_RIGHT;}

		case KEY_LEFT: {return K_TURN_LEFT;}
		case KEY_RIGHT: {return K_TURN_RIGHT;}

		default: {return NUM_KEYS; /* Invalid. */}
	}
}



#define MAX_EVENTS_PER_FRAME 64
void io_pollEvents(void) {
	struct input_event ev;
	size_t count = 0;

	while (count < MAX_EVENTS_PER_FRAME) {
		ssize_t n = read(fd, &ev, sizeof(ev));

		if (n > 0) {
			if (n != sizeof(ev)) {continue;}

			if (ev.type == EV_KEY) {
				int ID = getID(ev.code);
				if (ID < NUM_KEYS) {keyMap[ID] = (ev.value != 0); /* Update keyMap */}
			}

			count++;
		}
		else {
			if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {break; /* Stop, ran out. */}
			if (errno == EINTR) {continue; /* Interrupted, retry. */}

			//Error;
			perror("read");
			break;
		}
	}
}








#else //Windows only;

#include <windows.h>

static int getID(int vk) {
	switch (vk) {
		case VK_ESCAPE: {return K_QUIT;}

		case 'W': {return K_MOVE_FORE;}
		case 'S': {return K_MOVE_BACK;}
		case 'A': {return K_MOVE_LEFT;}
		case 'D': {return K_MOVE_RIGHT;}

		case VK_LEFT:  {return K_TURN_LEFT;}
		case VK_RIGHT: {return K_TURN_RIGHT;}

		default: {return NUM_KEYS;}
	}
}

void io_init(void) {/* Nothing to do. */}
void io_quit(void) {/* Nothing to do. */}

void io_pollEvents(void) {
	for (int vk = 0; vk < 256; vk++) {
		int ID = getID(vk);
		if (ID >= NUM_KEYS) {continue;}

		SHORT state = GetAsyncKeyState(vk);

		//High bit → key currently pressed
		keyMap[ID] = ((state & 0x8000) != 0);
	}
}


#endif