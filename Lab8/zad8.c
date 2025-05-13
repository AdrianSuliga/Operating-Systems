#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

#define N 3  // liczba użytkowników
#define M 2  // liczba drukarek
#define QUEUE_SIZE 5

typedef struct {
    char jobs[QUEUE_SIZE][11];
    int head;
    int tail;
    int count;
} PrintQueue;

sem_t queue_mutex;
sem_t queue_slots;
sem_t queue_items;

PrintQueue queue;

void random_string(char *str, int length) {
    for (int i = 0; i < length; ++i)
        str[i] = 'a' + rand() % 26;
    str[length] = '\0';
}

void *user_thread(void *arg) {
    int id = *(int *)arg;
    free(arg);
    char job[11];

    while (1) {
        random_string(job, 10);
        sem_wait(&queue_slots);
        sem_wait(&queue_mutex);

        strcpy(queue.jobs[queue.tail], job);
        queue.tail = (queue.tail + 1) % QUEUE_SIZE;
        queue.count++;

        printf("[Użytkownik %d] Wysłano zadanie: %s\n", id, job);
        fflush(stdout);

        sem_post(&queue_mutex);
        sem_post(&queue_items);

        sleep(5 + rand() % 10);
    }
}

void *printer_thread(void *arg) {
    int id = *(int *)arg;
    free(arg);
    char job[11];

    while (1) {
        sem_wait(&queue_items);
        sem_wait(&queue_mutex);

        strcpy(job, queue.jobs[queue.head]);
        queue.head = (queue.head + 1) % QUEUE_SIZE;
        queue.count--;

        sem_post(&queue_mutex);
        sem_post(&queue_slots);

        printf("[Drukarka %d] Rozpoczęto wydruk: %s\n", id, job);
        fflush(stdout);

        for (int i = 0; i < strlen(job); ++i) {
            printf("[Drukarka %d] %c\n", id, job[i]);
            fflush(stdout);
            sleep(1);
        }
    }
}

int main() {
    srand(time(NULL));

    memset(&queue, 0, sizeof(PrintQueue));

    sem_init(&queue_mutex, 0, 1);
    sem_init(&queue_slots, 0, QUEUE_SIZE);
    sem_init(&queue_items, 0, 0);

    pthread_t users[N], printers[M];

    for (int i = 0; i < N; ++i) {
        int *id = malloc(sizeof(int));
        *id = i + 1;
        pthread_create(&users[i], NULL, user_thread, id);
    }

    for (int i = 0; i < M; ++i) {
        int *id = malloc(sizeof(int));
        *id = i + 1;
        pthread_create(&printers[i], NULL, printer_thread, id);
    }

    for (int i = 0; i < N; ++i)
        pthread_join(users[i], NULL);

    for (int i = 0; i < M; ++i)
        pthread_join(printers[i], NULL);

    sem_destroy(&queue_mutex);
    sem_destroy(&queue_slots);
    sem_destroy(&queue_items);

    return 0;
}
