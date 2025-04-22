#include "common.h"
#include <time.h>

mqd_t client_queue;
mqd_t server_queue;
char client_queue_name[MAX_NAME_LENGTH];
int client_id = -1;
volatile sig_atomic_t running = 1;

// Obsługa przerwania
void handle_signal(int sig) {
    running = 0;
}

// Funkcja odbierająca wiadomości od serwera (uruchamiana w procesie potomnym)
void message_receiver() {
    Message msg;
    
    while (running) {
        ssize_t bytes_read = mq_receive(client_queue, (char*)&msg, sizeof(Message), NULL);
        
        if (bytes_read == -1) {
            if (errno == EINTR) {
                continue; // Przerwane przez sygnał
            }
            perror("Error receiving message from server");
            continue;
        }
        
        if (msg.type == INIT) {
            client_id = msg.client_id;
            printf("Connected to server. Your ID: %d\n", client_id);
        } else if (msg.type == MESSAGE) {
            printf("Client %d: %s\n", msg.client_id, msg.content);
        }
    }
}

int main() {
    // Ustawienie obsługi sygnałów
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    
    // Tworzenie nazwy kolejki klienta
    snprintf(client_queue_name, MAX_NAME_LENGTH, "/chat_client_%d_%ld", getpid(), time(NULL));
    
    // Inicjalizacja atrybutów kolejki
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = sizeof(Message);
    attr.mq_curmsgs = 0;
    
    // Utworzenie kolejki klienta
    mq_unlink(client_queue_name); // Usunięcie w razie gdyby już istniała
    client_queue = mq_open(client_queue_name, O_CREAT | O_RDONLY, 0666, &attr);
    if (client_queue == -1) {
        perror("Error creating client queue");
        exit(1);
    }
    
    // Otwieranie kolejki serwera
    server_queue = mq_open(SERVER_QUEUE_NAME, O_WRONLY);
    if (server_queue == -1) {
        perror("Error opening server queue");
        mq_close(client_queue);
        mq_unlink(client_queue_name);
        exit(1);
    }
    
    // Wysłanie komunikatu INIT do serwera
    Message init_msg;
    init_msg.type = INIT;
    init_msg.client_id = 0;
    strncpy(init_msg.client_queue, client_queue_name, MAX_NAME_LENGTH);
    strcpy(init_msg.content, "INIT");
    
    if (mq_send(server_queue, (char*)&init_msg, sizeof(Message), 0) == -1) {
        perror("Error sending INIT message");
        mq_close(client_queue);
        mq_unlink(client_queue_name);
        mq_close(server_queue);
        exit(1);
    }
    
    // Tworzenie procesu potomnego do odbierania wiadomości
    pid_t child_pid = fork();
    if (child_pid == -1) {
        perror("Error creating child process");
        mq_close(client_queue);
        mq_unlink(client_queue_name);
        mq_close(server_queue);
        exit(1);
    } else if (child_pid == 0) {
        // Proces potomny - odbiera wiadomości
        message_receiver();
        exit(0);
    }
    
    // Proces rodzicielski - wysyła wiadomości
    char buffer[MAX_MSG_SIZE];
    Message msg;
    
    printf("Chat client started. Type your messages (max %d characters):\n", MAX_MSG_SIZE - 1);
    
    while (running) {
        // Czekanie na inicjalizację klienta
        if (client_id == -1) {
            sleep(1);
            continue;
        }
        
        // Czytanie wiadomości z wejścia standardowego
        if (fgets(buffer, MAX_MSG_SIZE, stdin) == NULL) {
            break;
        }
        
        // Usunięcie znaku nowej linii
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len-1] == '\n') {
            buffer[len-1] = '\0';
        }
        
        // Sprawdzenie, czy użytkownik chce zakończyć
        if (strcmp(buffer, "exit") == 0 || strcmp(buffer, "quit") == 0) {
            break;
        }
        
        // Przygotowanie i wysłanie wiadomości
        msg.type = MESSAGE;
        msg.client_id = client_id;
        strncpy(msg.content, buffer, MAX_MSG_SIZE);
        
        if (mq_send(server_queue, (char*)&msg, sizeof(Message), 0) == -1) {
            perror("Error sending message");
        }
    }
    
    // Sprzątanie
    kill(child_pid, SIGTERM); // Zamknięcie procesu potomnego
    waitpid(child_pid, NULL, 0);
    
    mq_close(client_queue);
    mq_unlink(client_queue_name);
    mq_close(server_queue);
    
    return 0;
}