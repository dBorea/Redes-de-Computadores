#include "common.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <arpa/inet.h>

// UTILIDADES

void logexit(const char *msg){
    perror(msg);
    exit(EXIT_FAILURE);
}

bool StartsWith(const char *a, const char *b){
   if(strncmp(a, b, strlen(b)) == 0) return 1;
   return 0;
}

char *strremove(char *str, const char *sub){
    size_t len = strlen(sub);
    if (len > 0) {
        char *p = str;
        while ((p = strstr(p, sub)) != NULL) {
            memmove(p, p + len, strlen(p + len) + 1);
        }
    }
    return str;
}

Message* buildMessage(int tp, char* pld){
    Message *msg = malloc(sizeof(Message));
    msg->type = tp;
    strcpy(msg->payloadstr, pld);
    return msg;
}

void changeMessage(Message* msg, int tp, char* pld){
    msg->type = tp;
    strcpy(msg->payloadstr, pld);
}

int msgTypeFromString(char* str){
    if(StartsWith(str, "REQ_ADD")) { return REQ_ADD; }
    if(StartsWith(str, "REQ_INFOSE")) { return REQ_INFOSE; }
    if(StartsWith(str, "REQ_STATUS")) { return REQ_STATUS; }
    if(StartsWith(str, "REQ_INFOSCII")) { return REQ_INFOSCII; }
    if(StartsWith(str, "REQ_UP")) { return REQ_UP; }
    if(StartsWith(str, "REQ_NONE")) { return REQ_NONE; }
    if(StartsWith(str, "REQ_DOWN")) { return REQ_DOWN; }
    if(StartsWith(str, "RES_ADD")) { strremove(str, "RES_ADD "); return RES_ADD; }
    if(StartsWith(str, "REQ_REM")) { strremove(str, "REQ_REM "); return REQ_REM; }
    if(StartsWith(str, "RES_INFOSE")) { strremove(str, "RES_INFOSE "); return RES_INFOSE; }
    if(StartsWith(str, "RES_INFOSCII")) { strremove(str, "RES_INFOSCII "); return RES_INFOSCII; }
    if(StartsWith(str, "RES_STATUS")) { strremove(str, "RES_STATUS "); return RES_STATUS; }
    if(StartsWith(str, "RES_UP")) { strremove(str, "RES_UP "); return RES_UP; }
    if(StartsWith(str, "RES_NONE")) { strremove(str, "RES_NONE "); return RES_NONE; }
    if(StartsWith(str, "RES_DOWN")) { strremove(str, "RES_DOWN "); return RES_DOWN; }
    if(StartsWith(str, "ERROR")) { strremove(str, "ERROR "); return ERROR; }
    if(StartsWith(str, "OK")) { strremove(str, "OK "); return OK; }
    return -1;
}

char* getMsgAsStr(Message* msg, size_t *msgSize){

    size_t size = MSGSZ;

    if(msg->type <= 6){
        size = (strlen(MessageTypeStr[msg->type])+2) * sizeof(char);
        char *buffer = (char*)malloc(size);
        memset(buffer, 0, size);

        strcpy(buffer, MessageTypeStr[msg->type]);
        strcat(buffer, "\n");
        if(msgSize != NULL) *msgSize = size;
        return buffer;
    } 
    else {
        size = (strlen(MessageTypeStr[msg->type]) + strlen(msg->payloadstr) + 2) * sizeof(char);
        char *buffer = (char*)malloc(size);
        memset(buffer, 0, size);

        strcpy(buffer, MessageTypeStr[msg->type]);
        strcat(buffer, " ");
        strcat(buffer, msg->payloadstr);
        if(msgSize != NULL) *msgSize = size;
        return buffer;
    }
}

int sendMessage(int socket, Message* msg){
    size_t msgSize;
    char* buffer = getMsgAsStr(msg, &msgSize);
    printf("[SENDING] %s", buffer);

    size_t count = send(socket, buffer, msgSize, 0);
    
    if(count != msgSize)
        return -1;
    
    return 0;
}

int getMessage(int socket, Message* msg, int* bitcount){
    char buffer[BUFSZ];
    memset(buffer, 0, BUFSZ);

    *bitcount = recv(socket, buffer, BUFSZ, 0);
    if(*bitcount <= 0){
        return -1;
    }

    msg->type = msgTypeFromString(buffer);
    //printf("msg type = %d \n", msg->type);
    if(msg->type > 6){
        strcpy(msg->payloadstr, buffer);
    } else { strcpy(msg->payloadstr, ""); }
    

    return 0;
}

// FUNÇÕES DE ESTRUTURA BÁSICA - NÃO ALTERAR

int addrparse(const char *addrstr, const char *portstr, struct sockaddr_storage *storage) {  
    if(addrstr == NULL || portstr == NULL){
        return -1;
    }

    uint16_t port = atoi(portstr);
    if(port == 0){
        return -1;
    }
    port = htons(port); // host to network short
    
    struct in_addr inaddr4; // 32-bit IP address
    if (inet_pton(AF_INET, addrstr, &inaddr4)) {
        struct sockaddr_in *addr4 = (struct sockaddr_in *)storage;
        addr4->sin_family = AF_INET;
        addr4->sin_port = port;
        addr4->sin_addr = inaddr4;
        return 0;
    }

    struct in6_addr inaddr6; // 128-bit IPv6 address
    if (inet_pton(AF_INET6, addrstr, &inaddr6)) {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)storage;
        addr6->sin6_family = AF_INET6;
        addr6->sin6_port = port;
        addr6->sin6_addr = inaddr6;
        memcpy(&(addr6->sin6_addr), &inaddr6, sizeof(inaddr6));
        return 0;
    }

    return -1;
}

void addrtostr(const struct sockaddr *addr, char *str, size_t strsize) {
    int version;
    char addrstr[INET6_ADDRSTRLEN + 1] = "";
    uint16_t port;

    if (addr->sa_family == AF_INET) {
        version = 4;
        struct sockaddr_in *addr4 = (struct sockaddr_in *)addr;
        if (!inet_ntop(AF_INET, &(addr4->sin_addr), addrstr,
                       INET6_ADDRSTRLEN + 1)) {
            exit(EXIT_FAILURE);
        }
        port = ntohs(addr4->sin_port); // network to host short
    } else if (addr->sa_family == AF_INET6) {
        version = 6;
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)addr;
        if (!inet_ntop(AF_INET6, &(addr6->sin6_addr), addrstr,
                       INET6_ADDRSTRLEN + 1)) {
            exit(EXIT_FAILURE);
        }
        port = ntohs(addr6->sin6_port); // network to host short
    } else {
        exit(EXIT_FAILURE);
    }
    if (str) {
        snprintf(str, strsize, "IPv%d %s %hu", version, addrstr, port);
    }
}

int server_sockaddr_init(const char *proto, const char *portstr,
                         struct sockaddr_storage *storage) {
    uint16_t port = (uint16_t)atoi(portstr); // unsigned short
    if (port == 0) {
        return -1;
    }
    port = htons(port); // host to network short

    memset(storage, 0, sizeof(*storage));
    if (0 == strcmp(proto, "v4")) {
        struct sockaddr_in *addr4 = (struct sockaddr_in *)storage;
        addr4->sin_family = AF_INET;
        addr4->sin_addr.s_addr = INADDR_ANY;
        addr4->sin_port = port;
        return 0;
    } else if (0 == strcmp(proto, "v6")) {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)storage;
        addr6->sin6_family = AF_INET6;
        addr6->sin6_addr = in6addr_any;
        addr6->sin6_port = port;
        return 0;
    } else {
        return -1;
    }
}