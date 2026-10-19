/* io.c */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "types.h"
#include "maths.h"
#include "io.h"



int keyMapPress[NUM_KEYS];
int keyMapHold[NUM_KEYS];

int keyMapPrevious[NUM_KEYS];
char lastPressedChar;


#ifndef _WIN32 //Linux only;

#include <linux/input.h>
#include <linux/input-event-codes.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <termios.h>
#include <string.h>
#include <libudev.h>


#define TEST_BIT(arr, bit) (arr[(bit)/8] & (1 << ((bit)%8)))
#define MAX_DEVICES 32u
int fds[MAX_DEVICES];
unsigned int numFDs = 0;


static struct termios oldTermios;

int io_init(void) {
	
	//Find relevant input device, using libudev.
	//Initialise liudev stuff
	struct udev* udev = udev_new();
	struct udev_enumerate* enumerate = udev_enumerate_new(udev);

	//Add all input devices;
	udev_enumerate_add_match_subsystem(enumerate, "input");
	udev_enumerate_scan_devices(enumerate);
	struct udev_list_entry* devices = udev_enumerate_get_list_entry(enumerate);

	int found = FALSE;
	struct udev_list_entry* dev_list_entry;
	udev_list_entry_foreach(dev_list_entry, devices) {
		const char* path = udev_list_entry_get_name(dev_list_entry);
		struct udev_device* dev = udev_device_new_from_syspath(udev, path);

		const char* devnode = udev_device_get_devnode(dev);
		if (!devnode) {continue;}

		int fd = open(devnode, O_RDONLY | O_NONBLOCK);
		if (fd < 0) {continue;}

		unsigned long evbits[(EV_MAX + 7) / 8];
		memset(evbits, 0, sizeof(evbits));

		if (ioctl(fd, EVIOCGBIT(0, sizeof(evbits)), evbits) < 0) {
			close(fd);
			continue;
		}

		if (!TEST_BIT(evbits, EV_KEY)) {
			close(fd);
			continue;
		}

		unsigned long keybits[(KEY_MAX + 7) / 8];
		memset(keybits, 0, sizeof(keybits));

		if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keybits)), keybits) < 0) {
			close(fd);
			continue;
		}

		if (numFDs < MAX_DEVICES) {
			fds[numFDs++] = fd;
			printf("Using input: %s\n", devnode);
			found = TRUE;
		} else {
			close(fd);
			break;
		}
		udev_device_unref(dev);
	}
	if (!found) {return 0; /* Faliure. */}




	//Disable terminal input processing
	struct termios raw;
	tcgetattr(STDIN_FILENO, &oldTermios);

	raw = oldTermios;
	raw.c_lflag &= ~(ICANON | ECHO);  //Disable ln buf & echo

	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

	return 1;
}


void io_quit(void) {
	//Restore terminal input
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &oldTermios);
	for (unsigned int fdID=0u; fdID<numFDs; fdID++) {close(fds[fdID]);}
}



int getID(int key) {
	//Get the ID from the key enum.
	switch(key) {
		case KEY_ESC: {return K_QUIT;}

		case KEY_W: {return K_MOVE_FORE;}
		case KEY_S: {return K_MOVE_BACK;}
		case KEY_A: {return K_MOVE_LEFT;}
		case KEY_D: {return K_MOVE_RIGHT;}

		case KEY_LEFTSHIFT: {return K_MOVE_FAST;}
		case KEY_LEFTCTRL: {return K_MOVE_SLOW;}
		case KEY_SPACE: {return K_MOVE_JUMP;}

		case KEY_LEFT: {return K_TURN_LEFT;}
		case KEY_RIGHT: {return K_TURN_RIGHT;}
		case KEY_UP: {return K_NAV_UP;}
		case KEY_DOWN: {return K_NAV_DOWN;}

		case KEY_Q: {return K_SCREENSHOT;}


	#ifdef DEBUG_DRAW_ORDER
		case KEY_2: {return K_DEBUG_DRAW_INC;}
		case KEY_3: {return K_DEBUG_DRAW_DEC;}
	#endif

		default: {return NUM_KEYS; /* Invalid. */}
	}
}


char getKeyChar(int key) {
    switch (key) {
        case KEY_A: {return 'a';}
        case KEY_B: {return 'b';}
        case KEY_C: {return 'c';}
        case KEY_D: {return 'd';}
        case KEY_E: {return 'e';}
        case KEY_F: {return 'f';}
        case KEY_G: {return 'g';}
        case KEY_H: {return 'h';}
        case KEY_I: {return 'i';}
        case KEY_J: {return 'j';}
        case KEY_K: {return 'k';}
        case KEY_L: {return 'l';}
        case KEY_M: {return 'm';}
        case KEY_N: {return 'n';}
        case KEY_O: {return 'o';}
        case KEY_P: {return 'p';}
        case KEY_Q: {return 'q';}
        case KEY_R: {return 'r';}
        case KEY_S: {return 's';}
        case KEY_T: {return 't';}
        case KEY_U: {return 'u';}
        case KEY_V: {return 'v';}
        case KEY_W: {return 'w';}
        case KEY_X: {return 'x';}
        case KEY_Y: {return 'y';}
        case KEY_Z: {return 'z';}

        case KEY_COMMA: {return ',';}
        case KEY_DOT:   {return '.';}
        case KEY_SLASH: {return '/';}
        case KEY_MINUS: {return '-';}

        case KEY_0: {return '0';}
        case KEY_1: {return '1';}
        case KEY_2: {return '2';}
        case KEY_3: {return '3';}
        case KEY_4: {return '4';}
        case KEY_5: {return '5';}
        case KEY_6: {return '6';}
        case KEY_7: {return '7';}
        case KEY_8: {return '8';}
        case KEY_9: {return '9';}

        case KEY_SPACE:     {return ' ';}
        case KEY_BACKSPACE: {return '\b';}
        case KEY_ENTER:     {return '\n';}

        default: {return '\0';}
    }
}


#define MAX_EVENTS_PER_FRAME 64
void io_pollEvents(void) {
	memcpy(keyMapPrevious, keyMapHold, sizeof(keyMapHold));
	memset(keyMapPress, 0, sizeof(keyMapPress));
	lastPressedChar = '\0';

	for (unsigned int fdID=0u; fdID<numFDs; fdID++) {
		struct input_event ev;
		size_t count = 0;

		while (count < MAX_EVENTS_PER_FRAME) {
			ssize_t n = read(fds[fdID], &ev, sizeof(ev));

			if (n > 0) {
				if (n != sizeof(ev)) {continue;}

				if (ev.type == EV_KEY) {
					int ID = getID(ev.code);
					if (ev.value == 1) {lastPressedChar = getKeyChar(ev.code);}
					if (ID < NUM_KEYS) {
						//Update keymap
						keyMapHold[ID] = (ev.value != 0);
					}
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

	for (unsigned int i=0u; i<NUM_KEYS; i++) {
	    keyMapPress[i] = keyMapHold[i] && !keyMapPrevious[i];
	}
}








#else //Windows only;

#include <windows.h>

static int getID(int key) {
	switch (key) {
		case key_ESCAPE: {return K_QUIT;}

		case 'W': {return K_MOVE_FORE;}
		case 'S': {return K_MOVE_BACK;}
		case 'A': {return K_MOVE_LEFT;}
		case 'D': {return K_MOVE_RIGHT;}

		case key_LSHIFT: {return K_MOVE_FAST;}
		case key_LCTRL: {return K_MOVE_SLOW;}
		case key_SPACE: {return K_MOVE_JUMP;}

		case key_LEFT:  {return K_TURN_LEFT;}
		case key_RIGHT: {return K_TURN_RIGHT;}
		case key_UP:  {return K_NAV_UP;}
		case key_DOWN: {return K_NAV_DOWN;}

		case 'Q': {return K_SCREENSHOT;}

		default: {return NUM_KEYS;}
	}
}


char getKeyChar(int key) {
    if ((key >= 'A') && (key <= 'Z')) {
        return (char)(key - 'A' + 'a');
    }

    if ((key >= '0') && (key <= '9')) {
        return (char)key;
    }

	switch (key) {
		case key_SPACE:  {return ' ';}
		case key_BACK:   {return '\b';}
		case key_RETURN: {return '\n';}
		case key_TAB:    {return '\t';}
	}

    return '\0';
}


void io_init(void) {/* Nothing to do. */}
void io_quit(void) {/* Nothing to do. */}

void io_pollEvents(void) {
	lastPressedChar = '\0';
	for (int key=0; key<256; key++) {
		int ID = getID(key);
		if (ID >= NUM_KEYS) {continue;}

		SHORT state = GetAsyncKeyState(key);
		lastPressedChar = getKeyChar(key);

		//High bit → key currently pressed
		keyMapPrevious[ID] = keyMapHold[ID];
		keyMapHold[ID] = ((state & 0x8000) != 0);
		keyMapPress[ID] = keyMapHold[ID] && !keyMapPrevious[ID];
	}
}


#endif



