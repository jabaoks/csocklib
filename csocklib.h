/*
 * csocklib.h
 *
 *  Created on: May 22, 2022
 *      Author: Alex
 */

#ifndef CSOCKLIB_H_
#define CSOCKLIB_H_

/*
 * Callback function pointer like follows:
 * int on_read(void **ptr, char *buf, int len) {
 * 		csock_close(ptr);
 * 		return 0;
 * }
 * parameter ptr points to internal data to pass to csock_write() or csock_close()
 * parameter *ptr points to user context data
 */
typedef int (*FPTR)(void **ptr, char *, int len);

/*
 * initializes and returns pointer to library instance
 * data_len is user context data returned in on_read() callback
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
 * write data to opened socket
 * */
int csock_write(void *ptr, char *, int len);
/*
 * close socket
 * */
void csock_close(void *ptr);
/*
 * library prints debug information like printf
 * */
int csock_deb(const char *fmt, ... );

#endif /* CSOCKLIB_H_ */
