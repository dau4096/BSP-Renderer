/* multithread.h */
#ifndef MULTITHREAD_H
#define MULTITHREAD_H


#include <pthread.h>


//////// MULTITHREADING ////////
#define MT_SUCCESS (NULL)
#define MT_FAIL ((void*)1)


int mt_createThread(void* (*threadFunction)(void*), pthread_t* thisThread);
int mt_waitForThread(const pthread_t thisThread);
int mt_terminateThread(const pthread_t thisThread);
pthread_t mt_getCurrentThreadID(void);
void mt_exit(void);
//////// MULTITHREADING ////////



//////// TERMINAL RENDERING ////////
void mt_setDrawReady(void);
void* mt_framebufferDrawLoop(void* arg);
//////// TERMINAL RENDERING ////////



#endif
