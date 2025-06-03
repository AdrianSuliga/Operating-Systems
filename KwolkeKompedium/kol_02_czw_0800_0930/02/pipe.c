#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <sched.h>

#define BUFFER_SIZE 10

int main(int argc, char const *argv[])
{
	int fd[2];
	pid_t pid;
	char sendBuffer	[BUFFER_SIZE] = "welcome!";
	char receiveBuff[BUFFER_SIZE];
	// Wyzeruj bufor receiveBuff
    memset(receiveBuff, 0, BUFFER_SIZE);

	// Utwórz anonimowy potok
	if (pipe(fd) == -1) {
		fprintf(stderr, "... :%s\n", strerror(errno));
		exit(1);
	}

	// Utwórz proces potomny
	pid = fork();
	if (pid < 0) {
		fprintf(stderr, "... :%s\n", strerror(errno));
		exit(1);
	} else if (pid > 0)	{ // proces nadrzędny
        close(fd[0]);
		write(fd[1], sendBuffer, BUFFER_SIZE);
		printf("parent process %d send:%s\n", getpid(), sendBuffer);
		close(fd[1]);
	} else { // proces potomny
		close(fd[1]);
		read(fd[0], receiveBuff, BUFFER_SIZE);
		printf("child process %d receive:%s\n", getpid(), receiveBuff);
		close(fd[0]);
	}
	return 0;
}

/*
gcc -o pipe pipe.c 
./pipe 
parent process send:welcome!
child  process receive:welcome
*/
