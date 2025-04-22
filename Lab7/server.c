#include "common.h"

// Struktura przechowująca informacje o podłączonych klientach
typedef struct {
    int id;                     // ID klienta
    char queue_name[MAX_NAME_LENGTH]; // Nazwa kolejki klienta
    mqd_t queue;                // Deskryptor kolejki klienta
    int active;                 // Czy klient jest aktywny
} Client;

Client clients[MAX_CLIENTS];
mqd_t server_queue;
int next_client_id = 1;
volatile sig_atomic_t running = 1;

// Inicjalizacja tablicy klientów
void init_clients() {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].id = 0;
        clients[i].active = 0;
        clients[i].queue = -1;
    }
}

// Dodawanie nowego klienta
int add_client(const char *queue_name) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!clients[i].active) {
            clients[i].id = next_client_id++;
            strncpy(clients[i].queue_name, queue_name, MAX_NAME_LENGTH);
            
            // Otwieranie kolejki klienta
            struct mq_attr attr;
            attr.mq_flags = 0;
            attr.mq_maxmsg = 10;
            attr.mq_msgsize = sizeof(Message);
            attr.mq_curmsgs = 0;
            
            clients[i].queue = mq_open(queue_name, O_WRONLY);
            if (clients[i].queue == -1) {
                perror("Error opening client queue");
                return -1;
            }
            
            clients[i].active = 1;
            printf("Added client %d with queue %s\n", clients[i].id, queue_name);
            return clients[i].id;
        }
    }
    return -1; // Nie ma miejsca na nowego klienta
}

// Wyszukiwanie klienta po ID
Client* find_client(int id) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && clients[i].id == id) {
            return &clients[i];
        }
    }
    return NULL;
}

// Obsługa przerwania
void handle_signal(int sig) {
    running = 0;
}

int main() {
    // Ustawienie obsługi sygnałów
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    
    Message msg;
    struct mq_attr attr;
    
    // Inicjalizacja atrybutów kolejki
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = sizeof(Message);
    attr.mq_curmsgs = 0;
    
    // Utworzenie kolejki serwera
    mq_unlink(SERVER_QUEUE_NAME); // Usunięcie w razie gdyby już istniała
    server_queue = mq_open(SERVER_QUEUE_NAME, O_CREAT | O_RDONLY, 0666, &attr);
    if (server_queue == -1) {
        perror("Error creating server queue");
        exit(1);
    }
    
    printf("Server started. Queue: %s\n", SERVER_QUEUE_NAME);
    
    // Inicjalizacja tablicy klientów
    init_clients();
    
    // Główna pętla serwera
    while (running) {
        // Odbieranie komunikatu
        ssize_t bytes_read = mq_receive(server_queue, (char*)&msg, sizeof(Message), NULL);
        
        if (bytes_read == -1) {
            if (errno == EINTR) {
                // Przerwane przez sygnał
                continue;
            }
            perror("Error receiving message");
            continue;
        }
        
        // Obsługa komunikatu w zależności od typu
        if (msg.type == INIT) {
            printf("Received INIT from client with queue %s\n", msg.client_queue);
            
            int client_id = add_client(msg.client_queue);
            if (client_id != -1) {
                // Wysłanie odpowiedzi z ID
                Message response;
                response.type = INIT;
                response.client_id = client_id;
                strcpy(response.content, "Connection established");
                
                Client* client = find_client(client_id);
                if (client != NULL) {
                    if (mq_send(client->queue, (char*)&response, sizeof(Message), 0) == -1) {
                        perror("Error sending response to client");
                    }
                }
            }
        } else if (msg.type == MESSAGE) {
            printf("Received MESSAGE from client %d: %s\n", msg.client_id, msg.content);
            
            // Przesłanie wiadomości do wszystkich pozostałych klientów
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].active && clients[i].id != msg.client_id) {
                    if (mq_send(clients[i].queue, (char*)&msg, sizeof(Message), 0) == -1) {
                        perror("Error forwarding message to client");
                        // Można oznaczać klientów jako nieaktywnych w przypadku błędu
                    }
                }
            }
        }
    }
    
    // Sprzątanie
    printf("Server shutting down...\n");
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active) {
            mq_close(clients[i].queue);
        }
    }
    
    mq_close(server_queue);
    mq_unlink(SERVER_QUEUE_NAME);
    
    return 0;
}