#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>
#include <errno.h>

#define MAX_CLIENTS 10
#define MAX_NAME_LEN 32
#define MAX_MSG_LEN 512
#define BUFFER_SIZE 1024
#define ALIVE_INTERVAL 30

typedef struct {
    int socket;
    char name[MAX_NAME_LEN];
    struct sockaddr_in address;
    int active;
    time_t last_alive;
} client_t;

client_t clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
int server_socket;

void get_current_time(char* buffer) {
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
}

void signal_handler(int sig) {
    printf("\nZamykanie serwera...\n");
    close(server_socket);
    exit(0);
}

int find_client_by_socket(int socket) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && clients[i].socket == socket) {
            return i;
        }
    }
    return -1;
}

int find_client_by_name(const char* name) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && strcmp(clients[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

void remove_client(int index) {
    if (index >= 0 && index < MAX_CLIENTS && clients[index].active) {
        printf("Usuwanie klienta: %s\n", clients[index].name);
        close(clients[index].socket);
        clients[index].active = 0;
        memset(clients[index].name, 0, MAX_NAME_LEN);
    }
}

void send_client_list(int client_socket) {
    char buffer[BUFFER_SIZE] = "LIST:";
    char temp[64];
    
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active) {
            snprintf(temp, sizeof(temp), "%s,", clients[i].name);
            strcat(buffer, temp);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    
    // Usuń ostatny przecinek
    int len = strlen(buffer);
    if (len > 5 && buffer[len-1] == ',') {
        buffer[len-1] = '\0';
    }
    
    send(client_socket, buffer, strlen(buffer), 0);
}

void broadcast_message(const char* sender, const char* message, int sender_socket) {
    char time_str[26];
    char full_message[BUFFER_SIZE];
    
    get_current_time(time_str);
    snprintf(full_message, sizeof(full_message), "[%s] %s: %s", time_str, sender, message);
    
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && clients[i].socket != sender_socket) {
            send(clients[i].socket, full_message, strlen(full_message), 0);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void send_private_message(const char* sender, const char* recipient, const char* message) {
    char time_str[26];
    char full_message[BUFFER_SIZE];
    
    get_current_time(time_str);
    snprintf(full_message, sizeof(full_message), "[%s] PRIVATE from %s: %s", time_str, sender, message);
    
    pthread_mutex_lock(&clients_mutex);
    int recipient_index = find_client_by_name(recipient);
    if (recipient_index != -1) {
        send(clients[recipient_index].socket, full_message, strlen(full_message), 0);
    }
    pthread_mutex_unlock(&clients_mutex);
}

void* handle_client(void* arg) {
    int client_socket = *(int*)arg;
    char buffer[BUFFER_SIZE];
    char client_name[MAX_NAME_LEN];
    int client_index = -1;
    
    // Odbierz nazwę klienta
    int bytes_received = recv(client_socket, client_name, MAX_NAME_LEN-1, 0);
    if (bytes_received <= 0) {
        close(client_socket);
        return NULL;
    }
    client_name[bytes_received] = '\0';
    
    // Sprawdź czy nazwa nie jest zajęta
    pthread_mutex_lock(&clients_mutex);
    if (find_client_by_name(client_name) != -1) {
        pthread_mutex_unlock(&clients_mutex);
        send(client_socket, "ERROR: Nazwa już zajęta", 24, 0);
        close(client_socket);
        return NULL;
    }
    
    // Znajdź wolne miejsce dla klienta
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!clients[i].active) {
            clients[i].socket = client_socket;
            strcpy(clients[i].name, client_name);
            clients[i].active = 1;
            clients[i].last_alive = time(NULL);
            client_index = i;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    
    if (client_index == -1) {
        send(client_socket, "ERROR: Serwer pełny", 19, 0);
        close(client_socket);
        return NULL;
    }
    
    send(client_socket, "OK", 2, 0);
    printf("Klient %s połączony\n", client_name);
    
    // Główna pętla obsługi klienta
    while (1) {
        bytes_received = recv(client_socket, buffer, BUFFER_SIZE-1, 0);
        if (bytes_received <= 0) {
            break;
        }
        
        buffer[bytes_received] = '\0';

        if (buffer[0] != 'A') {
            printf("Otrzymano od %s: %s\n", client_name, buffer);
        }

        // Aktualizuj czas ostatniej aktywności
        pthread_mutex_lock(&clients_mutex);
        clients[client_index].last_alive = time(NULL);
        pthread_mutex_unlock(&clients_mutex);
        
        if (strncmp(buffer, "LIST", 4) == 0) {
            send_client_list(client_socket);
        }
        else if (strncmp(buffer, "2ALL ", 5) == 0) {
            broadcast_message(client_name, buffer + 5, client_socket);
        }
        else if (strncmp(buffer, "2ONE ", 5) == 0) {
            char* space = strchr(buffer + 5, ' ');
            if (space) {
                *space = '\0';
                char* recipient = buffer + 5;
                char* message = space + 1;
                send_private_message(client_name, recipient, message);
            }
        }
        else if (strncmp(buffer, "STOP", 4) == 0) {
            break;
        }
        else if (strncmp(buffer, "ALIVE", 5) == 0) {
            send(client_socket, "ALIVE_ACK", 9, 0);
        }
    }
    
    // Usuń klienta
    pthread_mutex_lock(&clients_mutex);
    remove_client(client_index);
    pthread_mutex_unlock(&clients_mutex);
    
    return NULL;
}

void* alive_checker(void* arg) {
    while (1) {
        sleep(ALIVE_INTERVAL);
        
        pthread_mutex_lock(&clients_mutex);
        time_t now = time(NULL);
        
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].active && (now - clients[i].last_alive) > ALIVE_INTERVAL * 2) {
                printf("Klient %s nie odpowiada - usuwanie\n", clients[i].name);
                remove_client(i);
            }
            else if (clients[i].active) {
                // Wyślij ping
                send(clients[i].socket, "ALIVE", 5, 0);
            }
        }
        pthread_mutex_unlock(&clients_mutex);
    }
    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Użycie: %s <port>\n", argv[0]);
        return 1;
    }
    
    int port = atoi(argv[1]);
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    // Obsługa sygnału Ctrl+C
    signal(SIGINT, signal_handler);
    
    // Inicjalizacja tablicy klientów
    memset(clients, 0, sizeof(clients));
    
    // Tworzenie socketu
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Błąd tworzenia socketu");
        return 1;
    }
    
    // Ustawienie opcji socketu
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // Konfiguracja adresu serwera
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    // Bindowanie socketu
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Błąd bindowania");
        close(server_socket);
        return 1;
    }
    
    // Nasłuchiwanie
    if (listen(server_socket, MAX_CLIENTS) < 0) {
        perror("Błąd nasłuchiwania");
        close(server_socket);
        return 1;
    }
    
    printf("Serwer nasłuchuje na porcie %d\n", port);
    
    // Uruchomienie wątku sprawdzającego aktywność klientów
    pthread_t alive_thread;
    pthread_create(&alive_thread, NULL, alive_checker, NULL);
    
    // Główna pętla serwera
    while (1) {
        int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket < 0) {
            if (errno == EINTR) continue;
            perror("Błąd akceptowania połączenia");
            continue;
        }
        
        printf("Nowe połączenie z %s:%d\n", 
               inet_ntoa(client_addr.sin_addr), 
               ntohs(client_addr.sin_port));
        
        // Tworzenie wątku dla klienta
        pthread_t client_thread;
        int* client_sock_ptr = malloc(sizeof(int));
        *client_sock_ptr = client_socket;
        
        if (pthread_create(&client_thread, NULL, handle_client, client_sock_ptr) != 0) {
            perror("Błąd tworzenia wątku");
            close(client_socket);
            free(client_sock_ptr);
        }
        
        pthread_detach(client_thread);
    }
    
    close(server_socket);
    return 0;
}
