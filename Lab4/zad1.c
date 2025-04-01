#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    if (argc != 2) {
        printf("Niepoprawna ilość argumentów, oczekiwano 1, otrzymano %d\n", argc - 1);
        return 0;
    }

    int pcount = atoi(argv[1]);
    if (pcount <= 0) {
        printf("Podano błędną ilość procesów\n");
        return 0;
    }

    for (int i = 0; i < pcount; i++) { 
        pid_t pid = fork();

        if (pid == 0) {
            printf("Child Process PPID = %d, PID = %d\n", (int)getppid(), (int)getpid());
            exit(0);
        }

        int status = 0;
        wait(&status);
    }

    printf("Otrzymano %d argumentów\n", pcount);

    return 0;
}