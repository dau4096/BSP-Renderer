/* io.c */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "types.h"
#include "maths.h"
#include "io.h"



int keyMap[NUM_KEYS];


#ifndef _WIN32 //Linux only;

#include <linux/input.h>
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
		if (!devnode) { continue; }

		int fd = open(devnode, O_RDONLY | O_NONBLOCK);
		if (fd < 0) { continue; }

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
		case KEY_E: {return K_MOVE_UP;}
		case KEY_Q: {return K_MOVE_DOWN;}

		case KEY_LEFTSHIFT: {return K_MOVE_FAST;}
		case KEY_SPACE: {return K_MOVE_JUMP;}

		case KEY_LEFT: {return K_TURN_LEFT;}
		case KEY_RIGHT: {return K_TURN_RIGHT;}

		default: {return NUM_KEYS; /* Invalid. */}
	}
}



#define MAX_EVENTS_PER_FRAME 64
void io_pollEvents(void) {
	for (unsigned int fdID=0u; fdID<numFDs; fdID++) {
		struct input_event ev;
		size_t count = 0;

		while (count < MAX_EVENTS_PER_FRAME) {
			ssize_t n = read(fds[fdID], &ev, sizeof(ev));

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
		case 'E': {return K_MOVE_UP;}
		case 'Q': {return K_MOVE_DOWN;}

		case VK_LSHIFT: {return K_MOVE_FAST;}
		case VK_SPACE: {return K_MOVE_JUMP;}

		case VK_LEFT:  {return K_TURN_LEFT;}
		case VK_RIGHT: {return K_TURN_RIGHT;}

		default: {return NUM_KEYS;}
	}
}

void io_init(void) {/* Nothing to do. */}
void io_quit(void) {/* Nothing to do. */}

void io_pollEvents(void) {
	for (int vk=0; vk<256; vk++) {
		int ID = getID(vk);
		if (ID >= NUM_KEYS) {continue;}

		SHORT state = GetAsyncKeyState(vk);

		//High bit → key currently pressed
		keyMap[ID] = ((state & 0x8000) != 0);
	}
}


#endif




#define MOVEMENT_SPEED_BASE 2.0f /* In units/second */
#define TURNING_SPEED 2.75f/* In radians/second */
#define JUMP_SPEED 1.0f /* 1-time impulse */

int prevJump = FALSE;
Vec2f_t io_handleInputs(Camera_t* camera, double dt) {
	//Camera Controls
	if (keyMap[K_TURN_LEFT]) {camera->yaw -= TURNING_SPEED * dt;}
	if (keyMap[K_TURN_RIGHT]) {camera->yaw += TURNING_SPEED * dt;}
	camera->yaw = fmod(camera->yaw, 2.0f * M_PI);
	if (camera->yaw < 0.0f) {camera->yaw += 2.0f * M_PI;}

	camera->forward = (Vec2f_t){
		.x=sin(camera->yaw), .y=cos(camera->yaw)
	};


	//Move camera based on inputs.
	float movementSpeed = MOVEMENT_SPEED_BASE * dt;
	if (keyMap[K_MOVE_FAST]) {movementSpeed *= 2.5f;}
	Vec2f_t forward = v2f_mul(camera->forward, movementSpeed);
	Vec2f_t right = (Vec2f_t){.x=forward.y, .y=-forward.x};

	Vec2f_t moveDelta = (Vec2f_t){.x=0.0f, .y=0.0f};
	if (keyMap[K_MOVE_FORE]) {moveDelta = v2f_add(moveDelta, forward);}
	if (keyMap[K_MOVE_BACK]) {moveDelta = v2f_sub(moveDelta, forward);}
	if (keyMap[K_MOVE_LEFT]) {moveDelta = v2f_sub(moveDelta, right);}
	if (keyMap[K_MOVE_RIGHT]) {moveDelta = v2f_add(moveDelta, right);}
	if (keyMap[K_MOVE_UP]) {camera->Z += movementSpeed;}
	if (keyMap[K_MOVE_DOWN]) {camera->Z -= movementSpeed;}

	if (keyMap[K_MOVE_JUMP] && !prevJump) {camera->Zvelocity += JUMP_SPEED;}
	prevJump = keyMap[K_MOVE_JUMP];

	return moveDelta;
}