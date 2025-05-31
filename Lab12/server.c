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

int find_client_by_address(struct sockaddr_in* addr) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && 
            clients[i].address.sin_addr.s_addr == addr->sin_addr.s_addr &&
            clients[i].address.sin_port == addr->sin_port) {
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
        clients[index].active = 0;
        memset(clients[index].name, 0, MAX_NAME_LEN);
    }
}

void send_client_list(struct sockaddr_in* client_addr) {
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
    
    sendto(server_socket, buffer, strlen(buffer), 0, 
           (struct sockaddr*)client_addr, sizeof(*client_addr));
}

void broadcast_message(const char* sender, const char* message, struct sockaddr_in* sender_addr) {
    char time_str[26];
    char full_message[BUFFER_SIZE];
    
    get_current_time(time_str);
    snprintf(full_message, sizeof(full_message), "[%s] %s: %s", time_str, sender, message);
    
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && 
            !(clients[i].address.sin_addr.s_addr == sender_addr->sin_addr.s_addr &&
              clients[i].address.sin_port == sender_addr->sin_port)) {
            sendto(server_socket, full_message, strlen(full_message), 0,
                   (struct sockaddr*)&clients[i].address, sizeof(clients[i].address));
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
        sendto(server_socket, full_message, strlen(full_message), 0,
               (struct sockaddr*)&clients[recipient_index].address, sizeof(clients[recipient_index].address));
    }
    pthread_mutex_unlock(&clients_mutex);
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
                sendto(server_socket, "ALIVE", 5, 0,
                       (struct sockaddr*)&clients[i].address, sizeof(clients[i].address));
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
    char buffer[BUFFER_SIZE];
    
    // Obsługa sygnału Ctrl+C
    signal(SIGINT, signal_handler);
    
    // Inicjalizacja tablicy klientów
    memset(clients, 0, sizeof(clients));
    
    // Tworzenie socketu UDP
    server_socket = socket(AF_INET, SOCK_DGRAM, 0);
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
    
    printf("Serwer UDP nasłuchuje na porcie %d\n", port);
    
    // Uruchomienie wątku sprawdzającego aktywność klientów
    pthread_t alive_thread;
    pthread_create(&alive_thread, NULL, alive_checker, NULL);
    
    // Główna pętla serwera
    while (1) {
        int bytes_received = recvfrom(server_socket, buffer, BUFFER_SIZE-1, 0,
                                     (struct sockaddr*)&client_addr, &client_len);
        if (bytes_received <= 0) {
            continue;
        }
        
        buffer[bytes_received] = '\0';
        
        pthread_mutex_lock(&clients_mutex);
        int client_index = find_client_by_address(&client_addr);
        
        // Jeśli to nowy klient próbujący się zarejestrować
        if (client_index == -1 && strncmp(buffer, "REGISTER:", 9) == 0) {
            char* client_name = buffer + 9;
            
            // Sprawdź czy nazwa nie jest zajęta
            if (find_client_by_name(client_name) != -1) {
                const char msg[] = "ERROR: Nazwa już zajęta";
                sendto(server_socket, msg, sizeof(msg), 0,
                       (struct sockaddr*)&client_addr, sizeof(client_addr));
                pthread_mutex_unlock(&clients_mutex);
                continue;
            }
            
            // Znajdź wolne miejsce dla klienta
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (!clients[i].active) {
                    strcpy(clients[i].name, client_name);
                    clients[i].address = client_addr;
                    clients[i].active = 1;
                    clients[i].last_alive = time(NULL);
                    client_index = i;
                    break;
                }
            }
            
            if (client_index == -1) {
                sendto(server_socket, "ERROR: Serwer pełny", 19, 0,
                       (struct sockaddr*)&client_addr, sizeof(client_addr));
            } else {
                sendto(server_socket, "OK", 2, 0,
                       (struct sockaddr*)&client_addr, sizeof(client_addr));
                printf("Klient %s połączony z %s:%d\n", client_name,
                       inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
            }
            pthread_mutex_unlock(&clients_mutex);
            continue;
        }
        
        // Jeśli to istniejący klient
        if (client_index != -1) {
            // Aktualizuj czas ostatniej aktywności
            clients[client_index].last_alive = time(NULL);
            
            if (buffer[0] != 'A') {
                printf("Otrzymano od %s: %s\n", clients[client_index].name, buffer);
            }
            
            if (strncmp(buffer, "LIST", 4) == 0) {
                pthread_mutex_unlock(&clients_mutex);
                send_client_list(&client_addr);
                continue;
            }
            else if (strncmp(buffer, "2ALL ", 5) == 0) {
                char* sender_name = clients[client_index].name;
                pthread_mutex_unlock(&clients_mutex);
                broadcast_message(sender_name, buffer + 5, &client_addr);
                continue;
            }
            else if (strncmp(buffer, "2ONE ", 5) == 0) {
                char* space = strchr(buffer + 5, ' ');
                if (space) {
                    *space = '\0';
                    char* recipient = buffer + 5;
                    char* message = space + 1;
                    char* sender_name = clients[client_index].name;
                    pthread_mutex_unlock(&clients_mutex);
                    send_private_message(sender_name, recipient, message);
                    continue;
                }
            }
            else if (strncmp(buffer, "STOP", 4) == 0) {
                remove_client(client_index);
                pthread_mutex_unlock(&clients_mutex);
                continue;
            }
            else if (strncmp(buffer, "ALIVE", 5) == 0) {
                sendto(server_socket, "ALIVE_ACK", 9, 0,
                       (struct sockaddr*)&client_addr, sizeof(client_addr));
                pthread_mutex_unlock(&clients_mutex);
                continue;
            }
        }
        pthread_mutex_unlock(&clients_mutex);
    }
    
    close(server_socket);
    return 0;
}