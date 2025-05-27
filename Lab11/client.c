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

void signal_handler(int sig) {
    printf("\nWysyłanie STOP do serwera...\n");
    send(client_socket, "STOP", 4, 0);
    close(client_socket);
    exit(0);
}

void* receive_messages(void* arg) {
    char buffer[BUFFER_SIZE];
    int bytes_received;
    
    while (1) {
        bytes_received = recv(client_socket, buffer, BUFFER_SIZE-1, 0);
        if (bytes_received <= 0) {
            printf("Połączenie z serwerem zostało przerwane\n");
            break;
        }
        
        buffer[bytes_received] = '\0';
        
        if (strncmp(buffer, "ALIVE", 5) == 0) {
            // Odpowiedz na ping serwera
            send(client_socket, "ALIVE", 5, 0);
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
    
    // Tworzenie socketu
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("Błąd tworzenia socketu");
        return 1;
    }
    
    // Konfiguracja adresu serwera
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        printf("Nieprawidłowy adres IP\n");
        close(client_socket);
        return 1;
    }
    
    // Połączenie z serwerem
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Błąd połączenia z serwerem");
        close(client_socket);
        return 1;
    }
    
    // Wysłanie nazwy klienta
    send(client_socket, client_name, strlen(client_name), 0);
    
    // Oczekiwanie na potwierdzenie
    char response[BUFFER_SIZE];
    int bytes_received = recv(client_socket, response, BUFFER_SIZE-1, 0);
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
    printf("  LIST - lista aktywnych klientów\n");
    printf("  2ALL <wiadomość> - wiadomość do wszystkich\n");
    printf("  2ONE <nazwa> <wiadomość> - wiadomość prywatna\n");
    printf("  STOP - zakończenie\n\n");
    
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
            send(client_socket, input, strlen(input), 0);
            break;
        }
        
        send(client_socket, input, strlen(input), 0);
    }
    
    close(client_socket);
    return 0;
}