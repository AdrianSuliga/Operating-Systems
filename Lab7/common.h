#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>

#define SERVER_QUEUE_NAME "/chat_server_queue"
#define MAX_CLIENTS 10
#define MAX_MSG_SIZE 1024
#define MAX_NAME_LENGTH 32

// Typy komunikatów
#define INIT 1
#define MESSAGE 2

// Struktura komunikatu
typedef struct {
    long type;            // Typ komunikatu: INIT lub MESSAGE
    int client_id;        // ID klienta (nadawane przez serwer)
    char client_queue[MAX_NAME_LENGTH]; // Nazwa kolejki klienta
    char content[MAX_MSG_SIZE];  // Treść wiadomości
} Message;

#endif