/*
 * csocktrgt.c
 *
 *  Created on: May 22, 2022
 *      Author: Alex
 */
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "csocktrgt.h"


#define CS_LOCK pthread_mutex_t
#define CSOCK_FD int

void *cs_init(void) {
	void *ptr = malloc(sizeof(CS_LOCK));
	if(ptr) {
		pthread_mutex_init((CS_LOCK *)ptr, 0);
	}
	return ptr;
}

void cs_lock(void *ptr) {
    pthread_mutex_lock((CS_LOCK *)ptr);
}
void cs_unlock(void *ptr) {
    pthread_mutex_unlock((CS_LOCK *)ptr);
}

void cs_release(void *ptr) {
	pthread_mutex_destroy((CS_LOCK *)ptr);
	free(ptr);
}

