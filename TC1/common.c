#include "common.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <arpa/inet.h>

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

char *stringFromType(enum ActionType t){
    static char *strings[] = {"CAD_REQ", "INI_REQ", "DES_REQ", "ALT_REQ", "SAL_REQ", "SAL_RES", "INF_REQ", "INF_RES", "ERROR", "OK", "EXIT", "kill"};
    return strings[t];
}

int typeFromString(char* str){
    if(StartsWith(str, "ERROR")){ return ERROR; }
    if(StartsWith(str, "OK")){ return OK; }
    if(StartsWith(str, "CAD_REQ ")){ strremove(str, "CAD_REQ "); return CAD_REQ; }
    if(StartsWith(str, "INI_REQ ")){ strremove(str, "INI_REQ "); return INI_REQ; }
    if(StartsWith(str, "DES_REQ ")){ strremove(str, "DES_REQ "); return DES_REQ; }
    if(StartsWith(str, "ALT_REQ ")){ strremove(str, "ALT_REQ "); return ALT_REQ; }
    if(StartsWith(str, "SAL_REQ ")){ strremove(str, "SAL_REQ "); return SAL_REQ; }
    if(StartsWith(str, "INF_REQ")){ strremove(str, "INF_REQ"); return INF_REQ; }
    if(StartsWith(str, "EXIT ")){ strremove(str, "EXIT "); return EXIT; }
    if(StartsWith(str, "kill")){ strremove(str, "kill"); return kill; }
    return -1;
}

int send_message(int socket, Message* msg){
    char temp[10];
    memset(temp, 0, 10);
    size_t size = 256;
    char *buffer;
    

    if(msg->type == SAL_RES || msg->type == INF_RES){
        size = (strlen(msg->payloadstr) + 1) * sizeof(char);

        buffer = (char*)malloc(size);
        memset(buffer, 0, size);

        strcpy(buffer, msg->payloadstr);
    } else if(msg->type == INF_REQ || msg->type == kill){
        strcpy(temp, stringFromType(msg->type));
        size = (strlen(temp) + 2) * sizeof(char);

        buffer = (char*)malloc(size);
        memset(buffer, 0, size);

        strcpy(buffer, temp);
        strcat(buffer, "\n");
    } else {
        strcpy(temp, stringFromType(msg->type));
        size = (strlen(temp) + strlen(msg->payloadstr) + 2) * sizeof(char);

        buffer = (char*)malloc(size);
        memset(buffer, 0, size);

        strcpy(buffer, temp);
        strcat(buffer, " ");
        strcat(buffer, msg->payloadstr);
    }

    size_t count = send(socket, buffer, size, 0);
    //printf("[sending] %s[%d bytes] [%d expected?]\n", buffer, (int)count, (int)size);
    if(count != size){
        return -1;
    }
    return 0;
}

int get_message(int socket, Message* msg, int* bitcount){
    char temp[10];
    memset(temp, 0, 10);
    size_t size = sizeof(temp) + sizeof(msg->payloadstr);

    char buffer[size];
    memset(buffer, 0, size);

    *bitcount = recv(socket, buffer, size, 0);
    //int expsize = (strlen(buffer) + 1)*sizeof(char);
    //printf("[received] %s[%d bytes] [%d expected]\n", buffer, *bitcount, expsize);
    if(*bitcount ==  0){
        return -1;
    }

    if(msg->type != SAL_RES && msg->type != INF_RES){
        msg->type = typeFromString(buffer);
    }
    strcpy(msg->payloadstr, buffer);
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