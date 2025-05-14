#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>

#define MAX_CLIENTS 10
#define MSG_SIZE 512
#define INIT 1
#define MESSAGE 2

typedef struct {
    long mtype;
    int client_id;
    key_t client_queue;
    char text[MSG_SIZE];
} message;

int client_queues[MAX_CLIENTS] = {0};
int client_count = 0;

int main() {
    key_t server_key = ftok("server.c", 65);
    int server_qid = msgget(server_key, 0666 | IPC_CREAT);
    if (server_qid == -1) {
        perror("msgget");
        exit(1);
    }

    printf("Server started. Waiting for clients...\n");

    message msg;
    while (1) {
        if (msgrcv(server_qid, &msg, sizeof(message) - sizeof(long), 0, 0) == -1) {
            perror("msgrcv");
            continue;
        }

        if (msg.mtype == INIT) {
            if (client_count >= MAX_CLIENTS) {
                printf("Max clients reached.\n");
                continue;
            }

            int client_qid = msgget(msg.client_queue, 0666);
            if (client_qid == -1) {
                perror("msgget client");
                continue;
            }

            int assigned_id = client_count + 1;
            client_queues[client_count++] = client_qid;
            
            message reply;
            reply.mtype = 1;
            reply.client_id = assigned_id;
            sprintf(reply.text, "Your ID is %d", assigned_id);
            msgsnd(client_qid, &reply, sizeof(message) - sizeof(long), 0);

            printf("Client %d connected.\n", assigned_id);

        } else if (msg.mtype == MESSAGE) {
            printf("Received from %d: %s\n", msg.client_id, msg.text);

            for (int i = 0; i < client_count; ++i) {
                if (i + 1 == msg.client_id) continue;
                message forward;
                forward.mtype = 1;
                forward.client_id = msg.client_id;
                snprintf(forward.text, MSG_SIZE, "Client %d: %.500s", msg.client_id, msg.text);
                msgsnd(client_queues[i], &forward, sizeof(message) - sizeof(long), 0);
            }
        }
    }

    msgctl(server_qid, IPC_RMID, NULL);
    return 0;
}
