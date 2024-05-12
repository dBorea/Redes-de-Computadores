#pragma once

#include <stdlib.h>
#include <stdbool.h>
#include <arpa/inet.h>

#define BUFSZ 1024
#define MSGSZ 256

enum ArgType {
    SOLO_ID,
    FULL_INFO
};

enum ActionType {
    CAD_REQ,
    INI_REQ,
    DES_REQ,
    ALT_REQ,
    SAL_REQ,
    SAL_RES,
    INF_REQ,
    INF_RES,
    ERROR,
    OK,
    EXIT,
    kill
};

typedef struct Message{
    int type;
    char payloadstr[MSGSZ];
} Message;

char *strremove(char *str, const char *sub);

char *stringFromType(enum ActionType t);

int typeFromString(char* str);

int send_message(int socket, Message* msg);

int get_message(int socket, Message* msg, int* bitcount);

bool StartsWith(const char *a, const char *b);

void logexit(const char *msg);

int addrparse(const char *addrstr, const char *portstr,
              struct sockaddr_storage *storage);

void addrtostr(const struct sockaddr *addr, char *str, size_t strsize);

int server_sockaddr_init(const char *proto, const char *portstr,
                         struct sockaddr_storage *storage);