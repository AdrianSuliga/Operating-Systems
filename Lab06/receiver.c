#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>

double f(double x) {
    return 4.0 / (x * x + 1);
}

double integrate(double a, double b, double width) {
    double sum = 0.0;
    for (double x = a; x < b; x += width) {
        sum += f(x) * width;
    }
    return sum;
}

int main() {
    const char *fifo1 = "fifo1";
    const char *fifo2 = "fifo2";

    double a, b;
    int fd1 = open(fifo1, O_RDONLY);
    read(fd1, &a, sizeof(double));
    read(fd1, &b, sizeof(double));
    close(fd1);

    double result = integrate(a, b, 1e-7);

    int fd2 = open(fifo2, O_WRONLY);
    write(fd2, &result, sizeof(double));
    close(fd2);

    return 0;
}
