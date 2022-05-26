/*
 * msock.c
 *
 *  Created on: May 22, 2022
 *      Author: Alex
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "csocklib.h"

struct context_data {
	int state;
	unsigned long long rx_total, tx_total;
};

static void *lib_data = 0; /* pointer to library instance */

char * parseArg(char *s, char delim)
{
    char *ps = s;
    int str_literal = 0;
    if( *ps == '"' )
    {
        ps++;
        str_literal = 1;
    }
    while( *ps )
    {
        if( !str_literal )
        {
            if(*ps <= ' ' || *ps == delim)
            {
                while( *ps <= ' ' || *ps == delim )
                {
                    if ( !*ps ) return NULL;
                    ps++;
                }
                return ps;
            }
        }
        else
        {
            if( *ps == '"' )
            {
                ps++;
                while( *ps <= ' ' || *ps == delim )
                {
                    if ( !*ps ) return NULL;
                    ps++;
                }
                return ps;
            }
        }
        ps ++;
    }
    return 0;
}

char * parseOneArg(char *s, char *buf, int max_len, char delim)
{
    int str_literal = 0;
    char *ps = parseArg(s, delim);
    if(ps)
    {
        int i;
        if(*ps == '"')
        {
            str_literal = 1;
            ps++;
        }
        for(i=0; ps[i]; i++)
        {
            if( i >= max_len ) break;
            if(!str_literal)
            {
                if( ps[i] == delim )
                {
                    break;
                }
                if( ps[i] < ' ' )
                {
                    break;
                }
            }
            else
            {
                if(ps[i] == '"')
                {
                    i++;
                    break;
                }
            }
            buf[i] = ps[i];
        }
        buf[i] = 0;
        return ps+i;
    }
    return 0;
}

static void do_get(void **ptr, char *ps) {
	char s[200];
	int fd;
	char buf[0x10000];
	strcpy(s, "www");
	parseOneArg(ps, s+3, sizeof(s)-4, ' ');
	printf("arg [%s]\n", s);
	if(strcmp(s,"www/")==0) {
		strcat(s, "index.html");
	}
	fd = open(s, O_RDONLY);
	if(fd>0) {
		int len;
		len = sprintf(buf, "HTTP/1.1 200 OK\n"
			"Server: CSock\n"
			"Connection: close\n"
			"\n"
			);
		csock_write(ptr, buf, len);

		while(1) {
			int len1;
			len = read(fd, buf, sizeof(buf));
			if(len<=0)
				break;
			len1 = csock_write(ptr, buf, len);
		}
		close(fd);
	}
	else {

		csock_deb("open [%s] error %s\n", s, strerror(errno));
		getcwd(s, sizeof(s));
		csock_deb("cur dir [%s]\n", s);


	}
	csock_close(ptr);
}

int on_read(void **ptr, char *buf, int len){
	struct context_data *pc = (struct context_data *)*ptr;
	if(len>0) {
		buf[len] = 0;
		pc->rx_total += len;
		printf("on_read %s %llu\n", buf, pc->rx_total);
		if(strncmp(buf, "B ", 2)==0) {
			csock_broadcast(lib_data, buf+2, len-2);
		}
		if(strncmp(buf, "GET ", 4)==0) {
			do_get(ptr, buf);
		}
		else if(strncmp(buf, "POST ", 5)==0) {
			do_get(ptr, buf);
		}
	}
	return 0;
}

int main(int argc, char **argv) {

	printf("start %s\n", argv[0]);
	lib_data = csock_init( sizeof(struct context_data ) );
	csock_start(lib_data, 8080, 0, on_read, 0);
	csock_release(lib_data);
	return 0;
}
