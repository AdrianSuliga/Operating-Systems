#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define M_PI 3.141592653589793

#define FABS(x) ((x) < 0 ? -(x) : (x))

typedef struct {
    int thread_id;
    int total_threads;
    double dx;
    int num_intervals;
    double* results;
} ThreadData;

double func(double x) {
    return 4.0 / (x * x + 1);
}

void* integrate_range(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    double sum = 0.0;
    int start = data->thread_id * (data->num_intervals / data->total_threads);
    int end = (data->thread_id + 1) * (data->num_intervals / data->total_threads);

    if (data->thread_id == data->total_threads - 1) {
        end = data->num_intervals; // ostatni wątek może dostać więcej
    }

    for (int i = start; i < end; ++i) {
        double x = i * data->dx;
        sum += func(x) * data->dx;
    }

    data->results[data->thread_id] = sum;
    return NULL;
}

int main(int argc, char** argv) {
    if (argc != 3) {
        printf("Użycie: %s <szerokość prostokąta dx> <liczba_wątków>\n", argv[0]);
        return 1;
    }

    double dx = atof(argv[1]);
    int num_threads = atoi(argv[2]);

    if (dx <= 0.0 || num_threads <= 0) {
        printf("Błędne parametry wejściowe.\n");
        return 1;
    }

    int num_intervals = (int)(1.0 / dx);
    double* results = calloc(num_threads, sizeof(double));
    pthread_t* threads = malloc(num_threads * sizeof(pthread_t));
    ThreadData* thread_data = malloc(num_threads * sizeof(ThreadData));

    for (int i = 0; i < num_threads; ++i) {
        thread_data[i].thread_id = i;
        thread_data[i].total_threads = num_threads;
        thread_data[i].dx = dx;
        thread_data[i].num_intervals = num_intervals;
        thread_data[i].results = results;

        pthread_create(&threads[i], NULL, integrate_range, &thread_data[i]);
    }

    for (int i = 0; i < num_threads; ++i) {
        pthread_join(threads[i], NULL);
    }

    double total = 0.0;
    for (int i = 0; i < num_threads; ++i) {
        total += results[i];
    }

    printf("Wynik całki ≈ %.15f\n", total);
    printf("Błąd względem PI ≈ %.15f\n", FABS(total - M_PI));
    
    free(results);
    free(threads);
    free(thread_data);

    return 0;
}
