/*
 * csocklib.c
 *
 *  Created on: May 22, 2022
 *      Author: Alex
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>

#include <sys/socket.h>
#include <netinet/in.h>
 #include <arpa/inet.h>

#include "csocktrgt.h"
#include "csocklib.h"

#define MAX_BUF_LEN 0x1000
#define MAX_SOCK 0x10000

struct ONESOCK {
	void *data;
	char flags;
	int fd;
	char *buf;
};

struct CSOCK {
	int proto;
	int port;
	int lis_fd;
	void *cs;
	FPTR on_write;
	FPTR on_read;
	FPTR on_error;
	int data_len;
	struct ONESOCK s[MAX_SOCK];
};

static int csock_loop(void *lib_data);
static int csock_open(int proto, int port);

int csock_deb(const char *fmt, ... ) {
    int ret = 0;
    char buf[ 512 ];
    int len = 0;
    va_list arg;
    va_start( arg, fmt );
    ret = len + vsnprintf( buf+len, sizeof( buf )-len-4, fmt, arg );
    va_end( arg );
    if( ret > 0 )
    {
        if( buf[ret-1] != '\n') {
            buf[ret++] = '\n';
        }
        buf[ret] = 0;
        ret = write(2, buf, ret);
    }
    return ret;
}

void * csock_init(int data_len) {
	void *ptr;
	ptr = malloc(sizeof(struct CSOCK));
	if(ptr) {
		struct CSOCK *pc=(struct CSOCK *)ptr;
		memset(ptr, 0, sizeof(struct CSOCK));
		pc->cs = cs_init();
		pc->data_len = data_len;
		pc->proto = IPPROTO_TCP;
		csock_deb("%s", __PRETTY_FUNCTION__);
	}
	return ptr;
}

int csock_start(void *lib_data, int port, FPTR on_write, FPTR on_read, FPTR on_error) {
	struct CSOCK *pc=(struct CSOCK *)lib_data;
	if(pc) {
		pc->on_write = on_write;
		pc->on_read = on_read;
		pc->on_error = on_error;
		pc->port = port;
		csock_deb("%s port %d", __PRETTY_FUNCTION__, port);
		csock_loop(pc);
		csock_release(pc);
	}
	return 0;
}

void csock_release(void *lib_data) {

	int i;
	struct CSOCK *pc=(struct CSOCK *)lib_data;
	cs_lock(pc->cs);
	for(i=0; i<MAX_SOCK; i++){
		struct ONESOCK *os = &pc->s[i];
		if(os->fd) {
			close(os->fd);
			os->fd = 0;
		}
		if(os->buf) {
			free(os->buf);
			os->buf = 0;
		}
		if(os->data) {
			free(os->data);
			os->data = 0;
		}
		os->flags = 0;
	}
	cs_unlock(pc->cs);
	free(pc);
}

int csock_broadcast(void *lib_data, char *buf, int len) {

	int ret;
	struct CSOCK *pc=(struct CSOCK *)lib_data;
	if(len>0) {
		int i;
		cs_lock(pc->cs);
		for(i=0; i<MAX_SOCK; i++) {
			struct ONESOCK *os =  &pc->s[i];
			if(os->flags && os->fd>0) {
				ret = write(os->fd, buf, len);
			}
		}
		cs_unlock(pc->cs);
	}
	return ret;
}

int csock_write(void *ptr, char *buf, int len) {
	struct ONESOCK *os = (struct ONESOCK *)ptr;
	int ret = write(os->fd, buf, len);
	return ret;
}

static struct ONESOCK *alloc_sock(struct CSOCK *pc) {
	int i;
	struct ONESOCK *os = 0;
	cs_lock(pc->cs);
	for(i=0; i<MAX_SOCK; i++){
		if(pc->s[i].flags == 0) {
			os = &pc->s[i];
			os->flags = 1;
			break;
		}
	}
	cs_unlock(pc->cs);
	return os;
}

static int csock_open(int proto, int port) {
	int ret;
    struct sockaddr_in addr;
    int flag = 1;
    ret = socket( AF_INET, SOCK_STREAM, proto );
    setsockopt(ret, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));

    if ((ret == -1)) {
    	close(ret);
        return -1;
    }
    memset(&addr, 0, sizeof(addr));
    addr.sin_port = htons(port);
    addr.sin_family = AF_INET;

    if( bind( ret, (struct sockaddr*) &addr, sizeof(addr)) == -1 ) {
        close(ret);
        return -1;
    }
    if( listen( ret, MAX_SOCK ) == -1 ) {
    	close(ret);
        return -1;
    }
	return ret;
}

void csock_close(void *ptr) {
	struct ONESOCK *os = (struct ONESOCK *)ptr;
	csock_deb("close fd %d", os->fd );
	close(os->fd);
	os->fd = 0;
	os->flags = 0;
}

static int csock_loop(void *ptr) {
	struct CSOCK *pc=(struct CSOCK *)ptr;
	pc->lis_fd = csock_open(pc->proto, pc->port);
	csock_deb("listen on fd %d", pc->lis_fd);
	while(1) {
		int i, n;
		int cnt = 0;
		struct ONESOCK *os;
		fd_set fd_read, fd_error;
	    struct  timeval timeout;
	    timeout.tv_sec = 1;
	    timeout.tv_usec = 0;

        FD_ZERO(&fd_read);
        FD_ZERO(&fd_error);
        FD_SET(pc->lis_fd, &fd_read);
        FD_SET(pc->lis_fd, &fd_error);
        cnt++;

        for(i=0; i<MAX_SOCK; i++)
        {
			os = &pc->s[i];
            if(os->fd)
            {
                FD_SET(os->fd, &fd_read);
                FD_SET(os->fd, &fd_error);
                cnt++;
            }
        }
		n = select(FD_SETSIZE, &fd_read, 0, &fd_error, &timeout);
		if(n>=0 && n<=cnt) {
			if(FD_ISSET(pc->lis_fd, &fd_read)) {
				os = alloc_sock(pc);
				if(os) {
                    struct sockaddr_in addr;
                    int len = sizeof(addr);
                    char str[INET_ADDRSTRLEN] = {0};
					if(!os->buf) {
						os->buf = malloc(MAX_BUF_LEN);
					}
					if(!os->data && pc->data_len>0 ) {
						os->data = malloc(pc->data_len);
					}
					os->fd = accept(pc->lis_fd, (struct sockaddr *)&addr, &len);
					inet_ntop(AF_INET, &(addr.sin_addr), str, INET_ADDRSTRLEN);
					csock_deb("connected fd %d %s", os->fd, str );
				}

			}
			else if(FD_ISSET(pc->lis_fd, &fd_error)) {
				csock_deb("listen error fd %d", os->fd );
			}
			else {
		        for(i=0; i<MAX_SOCK; i++)
		        {
					os = &pc->s[i];
		            if(os->fd)
		            {
						if(FD_ISSET(os->fd, &fd_error)) {
							pc->on_error(os->data, os->buf, 0);
							csock_close(os);
						}
						else if(FD_ISSET(os->fd, &fd_read)) {
							int ret = read(os->fd, os->buf, MAX_BUF_LEN);
							if(ret>0) {
								pc->on_read(&os->data, os->buf, ret);
							}
							else {
								csock_close(os);
							}
						}
		            }
		        }
			}
		}
	}
	return 0;
}
