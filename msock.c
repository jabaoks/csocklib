/*
 * msock.c
 *
 *  Created on: May 22, 2022
 *      Author: Alex
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "csocklib.h"

struct context_data {
	int state;
	unsigned long long rx_total, tx_total;
};

static void *lib_data = 0; /* pointer to library instance */

int on_write(void *ptr, char *buf, int len){
	struct context_data *pc = (struct context_data *)ptr;
	if(len>0) {
		buf[len] = 0;
		pc->tx_total += len;
		printf("on_write %s %llu\n", buf, pc->tx_total);
	}
	return 0;
}

int on_read(void *ptr, char *buf, int len){
	struct context_data *pc = (struct context_data *)ptr;
	if(len>0) {
		buf[len] = 0;
		pc->rx_total += len;
		printf("on_read %s %llu\n", buf, pc->rx_total);
		if(strncmp(buf, "B ", 2)==0) {
			csock_broadcast(lib_data, buf+2, len-2);
		}
	}
	return 0;
}

int on_error(void *ptr, char *buf, int len){
	printf("on_error\n");
	return 0;
}

int main(int argc, char **argv) {

	printf("start %s\n", argv[0]);
	lib_data = csock_init( sizeof(struct context_data ) );
	csock_start(lib_data, 11111, on_write, on_read, on_error);
	csock_release(lib_data);
	return 0;
}
