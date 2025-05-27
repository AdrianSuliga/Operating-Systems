#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#define MAX_PATIENTS_IN_WAITING_ROOM 3
#define PATIENTS_PER_CONSULTATION 3

// Globalne zmienne
int patients_in_waiting_room = 0;
int patients_waiting_ids[MAX_PATIENTS_IN_WAITING_ROOM];
int total_patients;
int patients_finished = 0;

// Muteksy i zmienne warunkowe
pthread_mutex_t waiting_room_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t doctor_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t doctor_condition = PTHREAD_COND_INITIALIZER;

// Funkcja zwracająca aktualny czas w formacie [HH:MM:SS]
void get_current_time(char* buffer) {
    struct timeval tv;
    struct tm* tm_info;
    
    gettimeofday(&tv, NULL);
    tm_info = localtime(&tv.tv_sec);
    
    sprintf(buffer, "[%02d:%02d:%02d.%03ld]", 
            tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec, 
            tv.tv_usec / 1000);
}

// Funkcja wątku pacjenta
void* patient_thread(void* arg) {
    int patient_id = *(int*)arg;
    char time_str[20];
    
    while (1) {
        // Losowy czas przybycia (2-5s)
        int arrival_time = 2 + rand() % 4;
        get_current_time(time_str);
        printf("%s - Pacjent(%d): Ide do szpitala, bede za %d s\n", 
               time_str, patient_id, arrival_time);
        sleep(arrival_time);
        
        // Próba wejścia do poczekalni
        pthread_mutex_lock(&waiting_room_mutex);
        
        if (patients_in_waiting_room >= MAX_PATIENTS_IN_WAITING_ROOM) {
            // Za dużo pacjentów, wracam później
            pthread_mutex_unlock(&waiting_room_mutex);
            
            int wait_time = 2 + rand() % 4;
            get_current_time(time_str);
            printf("%s - Pacjent(%d): za dużo pacjentów, wracam później za %d s\n", 
                   time_str, patient_id, wait_time);
            sleep(wait_time);
            continue;
        }
        
        // Wchodzę do poczekalni
        patients_waiting_ids[patients_in_waiting_room] = patient_id;
        patients_in_waiting_room++;
        
        get_current_time(time_str);
        printf("%s - Pacjent(%d): czeka %d pacjentów na lekarza\n", 
               time_str, patient_id, patients_in_waiting_room);
        
        // Jeśli jestem trzecim pacjentem, budzę lekarza
        if (patients_in_waiting_room == PATIENTS_PER_CONSULTATION) {
            get_current_time(time_str);
            printf("%s - Pacjent(%d): budzę lekarza\n", time_str, patient_id);
            
            pthread_mutex_lock(&doctor_mutex);
            pthread_cond_signal(&doctor_condition);
            pthread_mutex_unlock(&doctor_mutex);
        }
        
        pthread_mutex_unlock(&waiting_room_mutex);
        
        // Czekam na konsultację (będę obudzony przez lekarza)
        pthread_mutex_lock(&doctor_mutex);
        while (patients_in_waiting_room > 0) {
            pthread_cond_wait(&doctor_condition, &doctor_mutex);
        }
        pthread_mutex_unlock(&doctor_mutex);
        
        // Kończę wizytę
        get_current_time(time_str);
        printf("%s - Pacjent(%d): kończę wizytę\n", time_str, patient_id);
        
        pthread_mutex_lock(&waiting_room_mutex);
        patients_finished++;
        pthread_mutex_unlock(&waiting_room_mutex);
        
        // Pacjent kończy (nie wraca ponownie)
        break;
    }
    
    return NULL;
}

// Funkcja wątku lekarza
void* doctor_thread(void* arg) {
    char time_str[20];
    
    while (1) {
        // Sprawdzam czy wszyscy pacjenci zostali obsłużeni
        pthread_mutex_lock(&waiting_room_mutex);
        if (patients_finished >= total_patients) {
            pthread_mutex_unlock(&waiting_room_mutex);
            get_current_time(time_str);
            printf("%s - Lekarz: wszyscy pacjenci obsłużeni, kończę pracę\n", time_str);
            break;
        }
        pthread_mutex_unlock(&waiting_room_mutex);
        
        // Śpię i czekam na sygnał
        pthread_mutex_lock(&doctor_mutex);
        get_current_time(time_str);
        printf("%s - Lekarz: zasypiam\n", time_str);
        
        pthread_cond_wait(&doctor_condition, &doctor_mutex);
        
        get_current_time(time_str);
        printf("%s - Lekarz: budzę się\n", time_str);
        pthread_mutex_unlock(&doctor_mutex);
        
        // Sprawdzam czy są pacjenci do konsultacji
        pthread_mutex_lock(&waiting_room_mutex);
        if (patients_in_waiting_room >= PATIENTS_PER_CONSULTATION) {
            // Konsultuję pacjentów
            get_current_time(time_str);
            printf("%s - Lekarz: konsultuję pacjentów %d, %d, %d\n", 
                   time_str, 
                   patients_waiting_ids[0], 
                   patients_waiting_ids[1], 
                   patients_waiting_ids[2]);
            
            // Opróżniam poczekalnię
            patients_in_waiting_room = 0;
            pthread_mutex_unlock(&waiting_room_mutex);
            
            // Konsultacja trwa 2-4s
            int consultation_time = 2 + rand() % 3;
            sleep(consultation_time);
            
            // Budzę pacjentów po konsultacji
            pthread_mutex_lock(&doctor_mutex);
            pthread_cond_broadcast(&doctor_condition);
            pthread_mutex_unlock(&doctor_mutex);
        } else {
            pthread_mutex_unlock(&waiting_room_mutex);
        }
    }
    
    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Użycie: %s <liczba_pacjentów>\n", argv[0]);
        return 1;
    }
    
    total_patients = atoi(argv[1]);
    if (total_patients <= 0) {
        printf("Liczba pacjentów musi być większa od 0\n");
        return 1;
    }
    
    // Inicjalizacja generatora liczb losowych
    srand(time(NULL));
    
    printf("Rozpoczynam symulację szpitala z %d pacjentami\n", total_patients);
    
    // Tworzenie wątków
    pthread_t doctor;
    pthread_t* patients = malloc(total_patients * sizeof(pthread_t));
    int* patient_ids = malloc(total_patients * sizeof(int));
    
    // Tworzenie wątku lekarza
    pthread_create(&doctor, NULL, doctor_thread, NULL);
    
    // Tworzenie wątków pacjentów
    for (int i = 0; i < total_patients; i++) {
        patient_ids[i] = i + 1;
        pthread_create(&patients[i], NULL, patient_thread, &patient_ids[i]);
    }
    
    // Oczekiwanie na zakończenie wszystkich wątków
    pthread_join(doctor, NULL);
    
    for (int i = 0; i < total_patients; i++) {
        pthread_join(patients[i], NULL);
    }
    
    printf("Symulacja zakończona\n");
    
    // Zwolnienie pamięci
    free(patients);
    free(patient_ids);
    
    // Zniszczenie muteksów i zmiennych warunkowych
    pthread_mutex_destroy(&waiting_room_mutex);
    pthread_mutex_destroy(&doctor_mutex);
    pthread_cond_destroy(&doctor_condition);
    
    return 0;
}