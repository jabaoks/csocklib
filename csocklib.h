/*
 * csocklib.h
 *
 *  Created on: May 22, 2022
 *      Author: Alex
 */

#ifndef CSOCKLIB_H_
#define CSOCKLIB_H_

typedef int (*FPTR)(void *, char *, int len);

/*
 * initializes and returns pointer to library instance
 * data_len is user context data returned in on_read, on_write, on_error hooks
 * user context data is separate for each opened socket
 * */
void *csock_init(int data_len);
/*
 * starts listening on selected port
 * */
int csock_start(void *lib_data, int port, FPTR on_write, FPTR on_read, FPTR on_error);
/*
 * releases library instance
 * */
void csock_release(void *lib_data);
/*
 * sends broadcast message to all opened sockets
 * */
int csock_broadcast(void *lib_data, char *, int len);
/*
 * library prints debug information like printf
 * */
int csock_deb(const char *fmt, ... );

#endif /* CSOCKLIB_H_ */
