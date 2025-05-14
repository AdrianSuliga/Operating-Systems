#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>

int global = 0;

int main(int argc, char **argv)
{
    int local = 0;
    int status;

    if (argc != 2) {
        printf("Wrong number of arguments give - expected 1, got %d\n", argc - 1);
        return 1;
    }

    pid_t pid = fork();

    if (pid < 0) {
        printf("Failed to start new process with error %d\n", pid);
        return pid;
    } else if (pid == 0) {
        global++;
        local++;
        printf("child pid = %d, parent pid = %d\n", (int)getpid(), (int)getppid());
        printf("child's local = %d, child's global = %d\n", local, global);
        execl("/bin/ls", "ls", argv[1], NULL);
        printf("Listing failed\n");
        exit(EXIT_FAILURE);
    } else {
        waitpid(pid, &status, 0);

        printf("parent process\n");
        printf("parent pid = %d, child pid = %d\n", (int)getpid(), (int)pid);
        printf("child exit code: %d\n", WEXITSTATUS(status));
        printf("Parent's local = %d, parent's global = %d\n", local, global);

        return WEXITSTATUS(status);
    }
}