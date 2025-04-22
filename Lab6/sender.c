#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

int main() {
    const char *fifo1 = "fifo1";
    const char *fifo2 = "fifo2";

    double a, b;
    printf("Podaj przedział całkowania [a b]: ");
    if (scanf("%lf %lf", &a, &b) != 2) {
        printf("Błędne dane wejściowe.\n");
        return 1;
    }

    mkfifo(fifo1, 0666);
    mkfifo(fifo2, 0666);

    int fd1 = open(fifo1, O_WRONLY);
    write(fd1, &a, sizeof(double));
    write(fd1, &b, sizeof(double));
    close(fd1);

    double result;
    int fd2 = open(fifo2, O_RDONLY);
    read(fd2, &result, sizeof(double));
    close(fd2);

    printf("Wynik całki na przedziale [%.2f, %.2f] = %.15f\n", a, b, result);

    return 0;
}
