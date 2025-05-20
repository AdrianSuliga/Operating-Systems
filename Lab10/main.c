#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

// Konfiguracja
#define DEFAULT_PATIENTS 20
#define DEFAULT_PHARMACISTS 3
#define PHARMACY_CAPACITY 6

// Zmienne globalne
volatile int medicine_count = 0;
volatile int waiting_patients = 0;
volatile int active_patients = 0;
volatile int active_pharmacists = 0;
volatile int waiting_pharmacist = 0;

pthread_mutex_t hospital_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t doctor_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t patient_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t pharmacist_cond = PTHREAD_COND_INITIALIZER;

char* get_current_time() {
    static char buffer[64];
    struct timeval tv;
    struct tm* tm_info;
    
    gettimeofday(&tv, NULL);
    tm_info = localtime(&tv.tv_sec);
    
    snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d.%03d",
         tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec, (int)(tv.tv_usec / 1000));
    
    return buffer;
}

int get_random_time(int min, int max) {
    return min + rand() % (max - min + 1);
}

void* doctor_routine(void* arg) {
    while (1) {
        pthread_mutex_lock(&hospital_mutex);

        while (!((waiting_patients >= 3 && medicine_count >= 3) || 
                 (waiting_pharmacist && medicine_count < 3))) {
            if (active_patients == 0 && active_pharmacists == 0) {
                pthread_mutex_unlock(&hospital_mutex);
                printf("[%s] - Lekarz: kończę pracę, wszyscy pacjenci zostali obsłużeni.\n", get_current_time());
                return NULL;
            }
            printf("[%s] - Lekarz: zasypiam.\n", get_current_time());
            pthread_cond_wait(&doctor_cond, &hospital_mutex);
        }

        printf("[%s] - Lekarz: budzę się.\n", get_current_time());

        if (waiting_patients >= 3 && medicine_count >= 3) {
            waiting_patients -= 3;
            medicine_count -= 3;
            printf("[%s] - Lekarz: konsultuję pacjentów ...\n", get_current_time());

            pthread_mutex_unlock(&hospital_mutex);
            sleep(get_random_time(2, 4));
            pthread_mutex_lock(&hospital_mutex);
            pthread_cond_broadcast(&patient_cond);
        } else if (waiting_pharmacist && medicine_count < 3) {
            printf("[%s] - Lekarz: przyjmuję dostawę leków\n", get_current_time());
            pthread_mutex_unlock(&hospital_mutex);
            sleep(get_random_time(1, 3));
            pthread_mutex_lock(&hospital_mutex);
            medicine_count = PHARMACY_CAPACITY;
            waiting_pharmacist = 0;
            pthread_cond_signal(&pharmacist_cond);
        }

        pthread_mutex_unlock(&hospital_mutex);
    }
    return NULL;
}

void* patient_routine(void* arg) {
    int id = *((int*)arg);
    free(arg);
    srand(time(NULL) + id);

    sleep(get_random_time(2, 5));
    printf("[%s] - Pacjent(%d): Ide do szpitala, bede za kilka sekund.\n", get_current_time(), id);

    while (1) {
        pthread_mutex_lock(&hospital_mutex);

        if (waiting_patients >= 3) {
            int delay = get_random_time(2, 5);
            printf("[%s] - Pacjent(%d): za dużo pacjentów, wracam później za %d s.\n", get_current_time(), id, delay);
            pthread_mutex_unlock(&hospital_mutex);
            sleep(delay);
            continue;
        }

        waiting_patients++;
        printf("[%s] - Pacjent(%d): czeka %d pacjentów na lekarza.\n", get_current_time(), id, waiting_patients);

        if (waiting_patients == 3 && medicine_count >= 3) {
            printf("[%s] - Pacjent(%d): budzę lekarza.\n", get_current_time(), id);
            pthread_cond_signal(&doctor_cond);
        }

        while (1) {
            pthread_cond_wait(&patient_cond, &hospital_mutex);
            break;
        }

        pthread_mutex_unlock(&hospital_mutex);
        printf("[%s] - Pacjent(%d): kończę wizytę.\n", get_current_time(), id);

        pthread_mutex_lock(&hospital_mutex);
        active_patients--;
        pthread_mutex_unlock(&hospital_mutex);
        break;
    }
    return NULL;
}

void* pharmacist_routine(void* arg) {
    int id = *((int*)arg);
    free(arg);
    srand(time(NULL) + id);

    sleep(get_random_time(5, 15));
    printf("[%s] - Farmaceuta(%d): ide do szpitala, bede za kilka sekund.\n", get_current_time(), id);

    pthread_mutex_lock(&hospital_mutex);
    while (medicine_count == PHARMACY_CAPACITY) {
        printf("[%s] - Farmaceuta(%d): czekam na oproznienie apteczki.\n", get_current_time(), id);
        pthread_cond_wait(&pharmacist_cond, &hospital_mutex);
    }

    if (medicine_count < 3) {
        waiting_pharmacist = 1;
        printf("[%s] - Farmaceuta(%d): budzę lekarza.\n", get_current_time(), id);
        pthread_cond_signal(&doctor_cond);
        pthread_cond_wait(&pharmacist_cond, &hospital_mutex);
    }

    printf("[%s] - Farmaceuta(%d): dostarczam leki.\n", get_current_time(), id);
    printf("[%s] - Farmaceuta(%d): zakończyłem dostawę.\n", get_current_time(), id);
    active_pharmacists--;
    pthread_mutex_unlock(&hospital_mutex);

    return NULL;
}

int main(int argc, char* argv[]) {
    int num_patients = DEFAULT_PATIENTS;
    int num_pharmacists = DEFAULT_PHARMACISTS;

    if (argc >= 3) {
        num_patients = atoi(argv[1]);
        num_pharmacists = atoi(argv[2]);
    }

    active_patients = num_patients;
    active_pharmacists = num_pharmacists;

    pthread_t doctor;
    pthread_t* patients = malloc(num_patients * sizeof(pthread_t));
    pthread_t* pharmacists = malloc(num_pharmacists * sizeof(pthread_t));

    pthread_create(&doctor, NULL, doctor_routine, NULL);

    for (int i = 0; i < num_patients; ++i) {
        int* id = malloc(sizeof(int));
        *id = i + 1;
        pthread_create(&patients[i], NULL, patient_routine, id);
    }

    for (int i = 0; i < num_pharmacists; ++i) {
        int* id = malloc(sizeof(int));
        *id = i + 1;
        pthread_create(&pharmacists[i], NULL, pharmacist_routine, id);
    }

    for (int i = 0; i < num_patients; ++i) {
        pthread_join(patients[i], NULL);
    }

    for (int i = 0; i < num_pharmacists; ++i) {
        pthread_join(pharmacists[i], NULL);
    }

    pthread_cond_broadcast(&doctor_cond);
    pthread_join(doctor, NULL);

    free(patients);
    free(pharmacists);
    return 0;
}
