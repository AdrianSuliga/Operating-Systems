#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>

#define MAX_NAME_LEN 32
#define BUFFER_SIZE 1024

int client_socket;
char client_name[MAX_NAME_LEN];
struct sockaddr_in server_addr;

void signal_handler(int sig) {
    printf("\nWysyłanie STOP do serwera...\n");
    sendto(client_socket, "STOP", 4, 0, 
           (struct sockaddr*)&server_addr, sizeof(server_addr));
    close(client_socket);
    exit(0);
}

void* receive_messages(void* arg) {
    char buffer[BUFFER_SIZE];
    int bytes_received;
    struct sockaddr_in from_addr;
    socklen_t from_len = sizeof(from_addr);
    
    while (1) {
        bytes_received = recvfrom(client_socket, buffer, BUFFER_SIZE-1, 0,
                                 (struct sockaddr*)&from_addr, &from_len);
        if (bytes_received <= 0) {
            printf("Błąd odbierania wiadomości\n");
            break;
        }
        
        buffer[bytes_received] = '\0';
        
        if (strncmp(buffer, "ALIVE", 5) == 0) {
            // Odpowiedz na ping serwera
            sendto(client_socket, "ALIVE", 5, 0,
                   (struct sockaddr*)&server_addr, sizeof(server_addr));
        }
        else {
            printf("%s\n", buffer);
        }
    }
    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        printf("Użycie: %s <nazwa> <adres_IP> <port>\n", argv[0]);
        return 1;
    }
    
    strcpy(client_name, argv[1]);
    char* server_ip = argv[2];
    int server_port = atoi(argv[3]);
    
    // Obsługa sygnału Ctrl+C
    signal(SIGINT, signal_handler);
    
    // Tworzenie socketu UDP
    client_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_socket < 0) {
        perror("Błąd tworzenia socketu");
        return 1;
    }
    
    // Konfiguracja adresu serwera
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        printf("Nieprawidłowy adres IP\n");
        close(client_socket);
        return 1;
    }
    
    // Rejestracja klienta na serwerze
    char register_msg[BUFFER_SIZE];
    snprintf(register_msg, sizeof(register_msg), "REGISTER:%s", client_name);
    sendto(client_socket, register_msg, strlen(register_msg), 0,
           (struct sockaddr*)&server_addr, sizeof(server_addr));
    
    // Oczekiwanie na potwierdzenie
    char response[BUFFER_SIZE];
    struct sockaddr_in from_addr;
    socklen_t from_len = sizeof(from_addr);
    int bytes_received = recvfrom(client_socket, response, BUFFER_SIZE - 1, 0,
                                 (struct sockaddr*)&from_addr, &from_len);
    if (bytes_received <= 0) {
        printf("Błąd komunikacji z serwerem\n");
        close(client_socket);
        return 1;
    }
    
    response[bytes_received] = '\0';
    if (strncmp(response, "OK", 2) != 0) {
        printf("Błąd rejestracji: %s\n", response);
        close(client_socket);
        return 1;
    }
    
    printf("Połączono z serwerem jako %s\n", client_name);
    printf("Dostępne komendy:\n");
    printf(" LIST - lista aktywnych klientów\n");
    printf(" 2ALL <wiadomość> - wiadomość do wszystkich\n");
    printf(" 2ONE <nazwa> <wiadomość> - wiadomość prywatna\n");
    printf(" STOP - zakończenie\n\n");
    
    // Uruchomienie wątku do odbierania wiadomości
    pthread_t receive_thread;
    pthread_create(&receive_thread, NULL, receive_messages, NULL);
    
    // Główna pętla wysyłania wiadomości
    char input[BUFFER_SIZE];
    while (1) {
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }
        
        // Usuń znak nowej linii
        input[strcspn(input, "\n")] = '\0';
        
        if (strlen(input) == 0) {
            continue;
        }
        
        if (strcmp(input, "STOP") == 0) {
            sendto(client_socket, input, strlen(input), 0,
                   (struct sockaddr*)&server_addr, sizeof(server_addr));
            break;
        }
        
        sendto(client_socket, input, strlen(input), 0,
               (struct sockaddr*)&server_addr, sizeof(server_addr));
    }
    
    close(client_socket);
    return 0;
}