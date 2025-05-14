#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

volatile sig_atomic_t mode = 0;
volatile sig_atomic_t count = 0;
volatile sig_atomic_t sender_pid = 0;
volatile sig_atomic_t printing_numbers = 0;

void handle_sigusr1(int sig, siginfo_t *info, void *context) {
    mode = info->si_value.sival_int;
    sender_pid = info->si_pid;
    count++;
    printing_numbers = 0;
    
    kill(sender_pid, SIGUSR1);
}

void handle_sigint(int sig) {
    printf("Wciśnięto CTRL+C\n");
}

void print_numbers() {
    int num = 1;
    while (printing_numbers) {
        printf("%d\n", num++);
        sleep(1);
    }
}

int main() {
    printf("Catcher PID: %d\n", getpid());

    struct sigaction sa;
    sa.sa_sigaction = handle_sigusr1;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        printf("Błąd w sigaction\n!");
        return 1;
    }

    while (1) {
        switch (mode) {
            case 1:
                printf("Liczba otrzymanych żądań zmiany trybu: %d\n", count);
                mode = 0;
                break;
            case 2:
                mode = 0;
                printing_numbers = 1;
                print_numbers();
                printing_numbers = 0;
                break;
            case 3:
                signal(SIGINT, SIG_IGN);
                mode = 0;
                break;
            case 4:
                signal(SIGINT, handle_sigint);
                mode = 0;
                break;
            case 5:
                printf("Zakończenie pracy catchera\n");
                exit(0);
            default:
                pause();
                break;
        }
    }

    return 0;
}