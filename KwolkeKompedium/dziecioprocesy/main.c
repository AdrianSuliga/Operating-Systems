#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h> 

void sighandler(int sig, siginfo_t *info, void *context)
{
    printf("Child got %d\n", info->si_value.sival_int);
}

int main(int argc, char* argv[]) {

    if(argc != 3){
        printf("Not a suitable number of program parameters\n");
        return 1;
    }

    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_sigaction = &sighandler;
    action.sa_flags = SA_SIGINFO;
    sigaction(SIGUSR1, &action, NULL);

    int child = fork();
    if (child == 0) {
        // Proces potomny

        // Zablokuj wszystkie sygnały poza SIGUSR1
        sigset_t mask;
        sigfillset(&mask);
        sigdelset(&mask, SIGUSR1);
        sigprocmask(SIG_SETMASK, &mask, NULL);

        // Czekaj na sygnał
        pause();

        exit(0); // zakończ proces potomny po obsłużeniu sygnału
    }
    else {
        // Proces rodzica

        // Wyślij sygnał do dziecka
        union sigval value;
        value.sival_int = atoi(argv[1]);
        sigqueue(child, atoi(argv[2]), value);
    }

    return 0;
}
