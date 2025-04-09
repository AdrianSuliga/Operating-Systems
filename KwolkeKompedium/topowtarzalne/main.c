#include <stdlib.h>	
#include <unistd.h>	
#include <signal.h>	
#include <stdio.h>	

#include <sys/types.h>
#include <sys/wait.h>

/* 
   proces rodzica tworzy wiele procesow potomnych
   i wykrywa ich za konczenie za pomoca sygnalu
   Kazdy proces pttomny wyswietla komunikat "I'm the child" i "exits" po uplywie n sekund,
   gdzie n "is the sequence in which it was forked" 
   
   Nalezy uzypelnic kod programu, a nastepnie uruchomic program. 
   Komunikaty wyswietlone w termninalu znajduja sie w pliku out.txt
*/

#define NUMPROCS 4		/* number of processes to fork */
int nprocs;				/* number of child processes   */

void catch(int snum);		/* signal handler */
void child(int n);			/* the child calls this */
void parent(int pid);		/* the parent calls this */

int main(int argc, char **argv) 
{
    int pid;					
    int i;

    nprocs = atoi(argv[1]);

    // detect child termination
    signal(SIGCHLD, catch);

    for (i = 0; i < nprocs; i++) {
        pid = fork();
        switch (pid) {
            case 0:	/* child */
                child(i + 1); // sequence number starts from 1
                break;
            case -1:	/* error */
                perror("fork");
                exit(1);
            default:	/* parent */
                parent(pid);
                break;
        }
    }

    printf("parent: going to sleep\n");

    /* do nothing forever, until nprocs becomes 0 */
    while (nprocs != 0) 
    {
        printf("parent: sleeping\n");
        sleep(1);	/* sleep to wait for signals */
    }

    printf("parent: exiting\n");
    exit(0);
}

void child(int n) {
    printf("\tchild[%d]: child pid=%d, sleeping for %d seconds\n", n, getpid(), n);
    sleep(n);							/* do nothing for n seconds */
    printf("\tchild[%d]: I'm exiting\n", n);
    exit(100 + n);						/* exit with a return code of 100+n */
}

void parent(int pid) {
    printf("parent: created child pid=%d\n", pid);
}

void catch(int snum) {
    int pid;
    int status;

    pid = wait(&status); // wait for any child to change state
    if (pid > 0) {
        printf("parent: child process pid=%d exited with value %d\n", pid, WEXITSTATUS(status));
        nprocs--;
    }

    signal(SIGCHLD, catch); // re-establish the handler (for portability)
}
