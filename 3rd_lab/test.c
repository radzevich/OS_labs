#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>

int main(int argc, char *argv[])
{
  	int fd[2];
  	int pid, ret;
  	fd_set set;
  	struct timeval tm;

  	pipe(fd);

  	pid = fork();

  	if (pid == 0)
  	{
  		char buf[] = "hello\0";
  		size_t len = 6;
  		close(fd[0]);
  		sleep(1);
  		write(fd[1], buf, len);
  	}
  	else if (pid > 0)
  	{
  		close(fd[1]);
  		while(1)
  		{
  			FD_ZERO(&set);
  			FD_SET(fd[0], &set);
  			tm.tv_sec = 2;
  			tm.tv_usec = 0;

  			ret = select(fd[0] + 1, &set, NULL, NULL, &tm);

  			if (ret > 0)
  			{
  				printf("there is data\n");
  				break;
  			}
  		}
  	}

  	return 0;
}