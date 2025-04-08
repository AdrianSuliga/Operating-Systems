#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <bits/sigaction.h>
#include <bits/types/sigset_t.h>

void sigusr1_interrupt_handler(int signum)
{
    printf("Otrzymano sygnał %d\n", signum);
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        printf("Expected 1 argument, got %d", argc - 1);
        return 1;
    }

    printf("Process PID: %d\n", (int)getpid());

    if (!strcmp(argv[1], "none")) {
        signal(SIGUSR1, SIG_DFL);
    } else if (!strcmp(argv[1], "ignore")) {
        signal(SIGUSR1, SIG_IGN);
    } else if (!strcmp(argv[1], "handler")) {
        signal(SIGUSR1, sigusr1_interrupt_handler);
    } else if (!strcmp(argv[1], "mask")) {
        sigset_t new_mask, old_mask;
        sigemptyset(&new_mask);
        sigaddset(&new_mask, SIGUSR1);
        sigprocmask(SIG_SETMASK, &new_mask, &old_mask);
    } else {
        printf("Unknown command, %s\n", argv[1]);
        return 1;
    }

    while (1);

    return 0;
}