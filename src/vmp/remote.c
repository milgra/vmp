#ifndef remote_h
#define remote_h

#include "mt_channel.c"

typedef struct _remote_t
{
    int       alive;
    int       command;
    pthread_t thread;
} remote_t;

void remote_listen(remote_t* remote);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "mt_log.c"
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define SERVER_FIFO "/tmp/vmp"

void* remote_thread(void* data)
{
    remote_t* remote = (remote_t*) data;
    char      buffer[2];

    int result = mkfifo(SERVER_FIFO, 0664);
    if (result == 0 || errno == EEXIST)
    {
	while (1)
	{
	    int fd = open(SERVER_FIFO, O_RDONLY);

	    if (fd > -1)
	    {
		while (1)
		{
		    memset(buffer, '\0', sizeof(buffer));

		    int bytes_read = read(fd, buffer, sizeof(buffer));

		    if (bytes_read == -1) perror("read");

		    remote->command = atoi(buffer);

		    if (close(fd) == -1) perror("close");

		    break;
		}
	    }
	    else mt_log_error("Fifo open error");
	}
    }
    else mt_log_error("Fifo creation error");

    return NULL;
}

void remote_listen(remote_t* remote)
{
    remote->alive   = 1;
    remote->command = 0;

    pthread_create(&remote->thread, NULL, &remote_thread, remote);
}

#endif
