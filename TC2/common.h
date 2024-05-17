#pragma once

#include <stdlib.h>
#include <stdbool.h>
#include <arpa/inet.h>

#define BUFSZ 1024
#define MSGSZ 256

enum MessageType {
    // No payload
    REQ_ADD,        // both servers
    REQ_INFOSE,     // only SE
    REQ_STATUS,     // only SE
    REQ_INFOSCII,   // only CII
    REQ_UP,         // only CII
    REQ_NONE,       // only CII
    REQ_DOWN,       // only CII

    // Sends payload
    REQ_REM,        // both servers
    RES_ADD,        // to client
    RES_INFOSE,     // to client
    RES_INFOSCII,   // to client
    RES_STATUS,     // to client
    RES_UP,         // to client
    RES_NONE,       // to client
    RES_DOWN,       // to client

    ERROR,          // to client
    OK              // to client
};

enum ServerType {
    SE,
    CII,
    BOTH_SERVERS
};

typedef struct Message{
    int type;
    char payloadstr[MSGSZ];

} Message;

bool StartsWith(const char *a, const char *b);

char *strremove(char *str, const char *sub);

Message* buildMessage(int tp, char* pld);

void changeMessage(Message* msg, int tp, char* pld);

char* getMsgAsStr(Message* msg, size_t* msgSize);

int sendMessage(int socket, Message* msg, bool areWePrinting);

int getMessage(int socket, Message* msg, int* bitcount, bool areWePrinting);

// Premades

void logexit(const char *msg);

int addrparse(const char *addrstr, const char *portstr,
              struct sockaddr_storage *storage);

void addrtostr(const struct sockaddr *addr, char *str, size_t strsize);

int server_sockaddr_init(const char *proto, const char *portstr,
                         struct sockaddr_storage *storage);