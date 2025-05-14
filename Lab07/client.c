#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>

#define MSG_SIZE 512
#define INIT 1
#define MESSAGE 2

typedef struct {
    long mtype;
    int client_id;
    key_t client_queue;
    char text[MSG_SIZE];
} message;

void receive_messages(int client_qid) {
    message msg;
    while (1) {
        if (msgrcv(client_qid, &msg, sizeof(message) - sizeof(long), 0, 0) != -1) {
            printf("\n%s\n> ", msg.text);
            fflush(stdout);
        }
    }
}

int main() {
    key_t server_key = ftok("server.c", 65);
    int server_qid = msgget(server_key, 0666);
    if (server_qid == -1) {
        perror("msgget server");
        exit(1);
    }

    key_t client_key = ftok("client.c", getpid());
    int client_qid = msgget(client_key, 0666 | IPC_CREAT);
    if (client_qid == -1) {
        perror("msgget client");
        exit(1);
    }

    message msg;
    msg.mtype = INIT;
    msg.client_queue = client_key;
    msgsnd(server_qid, &msg, sizeof(message) - sizeof(long), 0);

    msgrcv(client_qid, &msg, sizeof(message) - sizeof(long), 0, 0);
    int client_id = msg.client_id;
    printf("Connected as client %d\n", client_id);

    if (fork() == 0) {
        receive_messages(client_qid);
        exit(0);
    }

    char input[MSG_SIZE];
    while (1) {
        printf("> ");
        fgets(input, MSG_SIZE, stdin);
        input[strcspn(input, "\n")] = '\0';

        msg.mtype = MESSAGE;
        msg.client_id = client_id;
        strcpy(msg.text, input);
        msgsnd(server_qid, &msg, sizeof(message) - sizeof(long), 0);
    }

    msgctl(client_qid, IPC_RMID, NULL);
    return 0;
}
