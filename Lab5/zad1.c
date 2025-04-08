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
        printf("Expected 1 argument, got %d\n", argc - 1);
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
        sigset_t new_mask;
        sigemptyset(&new_mask);
        sigaddset(&new_mask, SIGUSR1);
        sigprocmask(SIG_BLOCK, &new_mask, NULL);

        sigset_t p;
        sigpending(&p);
        if (sigismember(&p, SIGUSR1)) {
            printf("Sygnał SIGUSR1 jest oczekujący\n");
        } else {
            printf("Sygnał SIGUSR1 nie jest oczekujący\n");
        }
    } else {
        printf("Unknown command, %s\n", argv[1]);
        return 1;
    }

    raise(SIGUSR1);

    if (!strcmp(argv[1], "mask")) {
        sigset_t pending;
        sigpending(&pending);
        if (sigismember(&pending, SIGUSR1)) {
            printf("Sygnał SIGUSR1 jest oczekujący\n");
        } else {
            printf("Sygnał SIGUSR1 nie jest oczekujący\n");
        }
    }

    return 0;
}