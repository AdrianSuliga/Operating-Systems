#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

volatile sig_atomic_t confirmation_received = 0;

void sigusr1_handler(int sig) {
    confirmation_received = 1;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        printf("Oczekiwano 2 argumentów ale otrzymano %d\n", argc);
        return 1;
    }

    pid_t catcher_pid = atoi(argv[1]);
    int mode = atoi(argv[2]);

    if (mode < 1 || mode > 5) {
        printf("Błędny tryb (1-5)\n");
        return 1;
    }

    struct sigaction sa;
    sa.sa_handler = sigusr1_handler;
    sa.sa_flags = 0;  
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        printf("Błąd w sigaction\n");
        return 1;
    }

    sigset_t block_mask, empty_mask;
    sigemptyset(&block_mask);
    sigaddset(&block_mask, SIGUSR1);
    
    if (sigprocmask(SIG_BLOCK, &block_mask, NULL) == -1) {
        printf("Błąd w sigprocmask\n");
        return 1;
    }

    union sigval value;
    value.sival_int = mode;
    if (sigqueue(catcher_pid, SIGUSR1, value) == -1) {
        printf("Błąd w sigqueue\n");
        return 1;
    }

    sigemptyset(&empty_mask);
    while (!confirmation_received) {
        sigsuspend(&empty_mask);  
    }

    printf("Uzyskano potwierdzenie otrzymania trybu %d\n", mode);

    return 0;
}