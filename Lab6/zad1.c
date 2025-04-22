#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <math.h>

double f(double x) {
    return 4.0 / (x * x + 1);
}

double get_time_in_seconds() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1e6;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Oczekiwano 2 argumentów, otrzymano %d\n", argc - 1);
        return 1;
    }

    double width = atof(argv[1]);
    int max_k = atoi(argv[2]);

    if (width <= 0 || max_k <= 0) {
        printf("Niepoprawne argumenty wejściowe.\n");
        return 1;
    }

    double a = 0.0, b = 1.0;
    int total_steps = (int)((b - a) / width);

    for (int k = 1; k <= max_k; k++) {
        int pipes[k][2];
        pid_t pids[k];

        double start_time = get_time_in_seconds();

        for (int i = 0; i < k; i++) {
            pipe(pipes[i]);
            pids[i] = fork();

            if (pids[i] == 0) {
                close(pipes[i][0]); 

                int steps_per_child = total_steps / k;
                int extra = total_steps % k;

                int start_index = i * steps_per_child + (i < extra ? i : extra);
                int steps = steps_per_child + (i < extra ? 1 : 0);

                double local_a = a + start_index * width;
                double local_sum = 0.0;

                for (int j = 0; j < steps; j++) {
                    double x = local_a + j * width;
                    local_sum += f(x) * width;
                }

                write(pipes[i][1], &local_sum, sizeof(double));
                close(pipes[i][1]);
                exit(0);
            }

            close(pipes[i][1]);
        }

        double result = 0.0;
        for (int i = 0; i < k; i++) {
            double partial = 0.0;
            read(pipes[i][0], &partial, sizeof(double));
            result += partial;
            close(pipes[i][0]);
        }

        for (int i = 0; i < k; i++) {
            waitpid(pids[i], NULL, 0);
        }

        double end_time = get_time_in_seconds();

        printf("k = %d, wynik = %.20f, czas = %.4f s\n", k, result, end_time - start_time);
    }

    return 0;
}