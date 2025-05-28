#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define MAX_WAITING_PATIENTS 3

volatile int waiting_patients = 0;
volatile int total_patients = 0;
volatile int remaining_patients = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t doctor_wakeup = PTHREAD_COND_INITIALIZER;
pthread_cond_t consultation_done = PTHREAD_COND_INITIALIZER;

void print_time() {
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    printf("[%02d:%02d:%02d] - ", t->tm_hour, t->tm_min, t->tm_sec);
}

void* patient_thread(void* arg) {
    int id = *(int*)arg;
    free(arg);

    int wait_time = rand() % 4 + 2;
    print_time(); printf("Pacjent(%d): Ide do szpitala, bede za %d s\n", id, wait_time);
    sleep(wait_time);

    while (1) {
        pthread_mutex_lock(&mutex);
        if (waiting_patients >= MAX_WAITING_PATIENTS) {
            int retry_time = rand() % 3 + 1;
            print_time(); printf("Pacjent(%d): za duzo pacjentow, wracam pozniej za %d s\n", id, retry_time);
            pthread_mutex_unlock(&mutex);
            sleep(retry_time);
            continue;
        }

        waiting_patients++;
        print_time(); printf("Pacjent(%d): czeka %d pacjentow na lekarza\n", id, waiting_patients);

        if (waiting_patients == MAX_WAITING_PATIENTS) {
            print_time(); printf("Pacjent(%d): budze lekarza\n", id);
            pthread_cond_signal(&doctor_wakeup);
        }

        pthread_cond_wait(&consultation_done, &mutex);
        pthread_mutex_unlock(&mutex);

        print_time(); printf("Pacjent(%d): koncze wizyte\n", id);
        break;
    }

    return NULL;
}

void* doctor_thread(void* arg) {
    while (1) {
        pthread_mutex_lock(&mutex);
        while (waiting_patients < MAX_WAITING_PATIENTS) {
            pthread_cond_wait(&doctor_wakeup, &mutex);
        }

        print_time(); printf("Lekarz: budze sie\n");

        print_time(); printf("Lekarz: konsultuje pacjentow\n");
        pthread_mutex_unlock(&mutex);
        sleep(rand() % 3 + 2); 
        pthread_mutex_lock(&mutex);

        waiting_patients = 0;
        remaining_patients -= 3;
        pthread_cond_broadcast(&consultation_done);

        print_time(); printf("Lekarz: zasypiam\n");
        pthread_mutex_unlock(&mutex);

        if (remaining_patients == 0) break;
    }
    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Uzycie: %s <liczba_pacjentow>\n", argv[0]);
        return 1;
    }

    total_patients = atoi(argv[1]);
    remaining_patients = total_patients;

    pthread_t doctor;
    pthread_t patients[total_patients];

    srand(time(NULL));
    pthread_create(&doctor, NULL, doctor_thread, NULL);

    for (int i = 0; i < total_patients; i++) {
        int* id = malloc(sizeof(int));
        *id = i + 1;
        pthread_create(&patients[i], NULL, patient_thread, id);
    }

    for (int i = 0; i < total_patients; i++) {
        pthread_join(patients[i], NULL);
    }

    pthread_mutex_lock(&mutex);
    pthread_cond_signal(&doctor_wakeup);
    pthread_mutex_unlock(&mutex);
    pthread_join(doctor, NULL);

    return 0;
}
