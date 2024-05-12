#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <bits/pthreadtypes.h>
#include <asm-generic/socket.h>

enum ServerType {
    SE,
    CII
};

typedef struct ClientData{
    int csock;
    struct sockaddr_storage storage;
} ClientData;

typedef struct ServerData{
    int clientIds[10];

    int server_type;
    int dataValue;

    ClientData *cdata;
} ServerData;

// Gera valor aleatório no range 20~50 MWh -- type: 1(aumentar) 0(neutro) -1(diminuir)
int randomPowerUsage(int type, ServerData *dados){
    int newValue;
    switch(type){
        case 1: // Número aleatório entre dataValue e 100
            newValue = dados->dataValue + rand() % (100 + 1 - dados->dataValue); 
            break;
        case 0: // Número aleatório entre 100 e 0
            newValue = rand() % (100 + 1);
            break;
        case -1: // Número aleatório entre 0 e dataValue
            newValue = rand() % (dados->dataValue + 1);
            break;
    }
    return newValue;
}

int randomPowerGen(){
    return 20 + rand() % (50 + 1 - 20);
}

int serverMsgHandler(ServerData *server_data, int socket, Message *msg){
    switch(msg->type){
        case REQ_ADD:
            printf("[MSG] Add request\n");
            break;
        case REQ_INFOSE:
            break;
        case REQ_STATUS:
            break;
        case REQ_INFOSCII:
            break;
        case REQ_UP:
            break;
        case REQ_NONE:
            break;
        case REQ_DOWN:
            break;
    }
    return -1;
}

void * SE_thread(void *data){
    ServerData *server_data = (struct ServerData *)data;
    struct ClientData *cdata = (struct ClientData *)(&server_data->cdata);
    struct sockaddr *caddr = (struct sockaddr *)(&cdata->storage);

    char caddrstr[BUFSZ];
    addrtostr(caddr, caddrstr, BUFSZ);
    printf("[SE] [log] connection from %s\n", caddrstr);
    printf("[SE] [log] Power Gen. initialized as %d\n", server_data->dataValue);

    Message *msg_in = buildMessage(-1, "");  

    while(1){ // Loop interno do server SE, gerencia as mensagens e a comunicação com o cliente
        // EXECUTION LOOP
        int bitcount;
        if(0 != getMessage(cdata->csock, msg_in, &bitcount)){
            printf("[log] client forcefully disconnected\n");
            close(cdata->csock);
            break;
        }
        //printf("[msg] (%d bytes) %s %s\n",(int)bitcount, MessageTypeStr[msg_in->type], msg_in->payloadstr);
        fflush(stdout);

        //serverMsgHandler(server_data, cdata->csock, msg_in);
    }

    pthread_exit(EXIT_SUCCESS);
}

void * CII_thread(void *data){
    ServerData *server_data = (struct ServerData *)data;
    struct ClientData *cdata = (struct ClientData *)(&server_data->cdata);
    struct sockaddr *caddr = (struct sockaddr *)(&cdata->storage);

    char caddrstr[BUFSZ];
    addrtostr(caddr, caddrstr, BUFSZ);
    printf("[CII] [log] connection from %s\n", caddrstr);
    printf("[CII] [log] Power Usage initialized as %d\n", server_data->dataValue);


    while(1){ // Loop interno do server SE, gerencia as mensagens e a comunicação com o cliente
        // EXECUTION LOOP
    }

    pthread_exit(EXIT_SUCCESS);
}

void usage(int argc, char **argv) {
    printf("usage: %s <v4/v6> <server port>\n", argv[0]);
    printf("example: %s v4 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
    srand(time(NULL));

    if(argc < 3){
        usage(argc, argv);
    }

    struct sockaddr_storage storage;
    if(0 != server_sockaddr_init(argv[1], argv[2], &storage)){
        usage(argc, argv);
    }

    ServerData *server_data = malloc(sizeof(ServerData));
    for(int i=0; i<10; i++) { server_data->clientIds[i] = 0; }

    if(strcmp(argv[2], "12345") == 0){
        printf("Server type [SE] ");
        server_data->server_type = SE;
        server_data->dataValue = randomPowerGen();
    } 
    else if (strcmp(argv[2], "54321") == 0){
        printf("Server type [CII] ");
        server_data->server_type = CII;
        server_data->dataValue = randomPowerUsage(0, server_data);
    } 
    else {
        logexit("Only ports \'12345\' and \'54321\' are accepted.\n");
    }

    int s;
    s = socket(storage.ss_family, SOCK_STREAM, 0);
    if(s == -1){
        logexit("socket");
    }

    int enable = 1;
    if(0 != setsockopt(s, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &enable, sizeof(int))){
        logexit("setsockopt");
    }

    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if(0 != bind(s, addr, sizeof(storage))){
        logexit("bind");
    }

    if(0 != listen(s, 1)){
        logexit("listen");
    }

    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ);
    printf("bound to %s, waiting connection\n", addrstr);

    bool running = true;

    while(running){ // Loop externo, apenas para conexões;
        struct sockaddr_storage c_storage;
        struct sockaddr *caddr = (struct sockaddr *)(&c_storage);
        socklen_t caddrlen = sizeof(c_storage);

        int csock = accept(s, caddr, &caddrlen);
        if(csock == -1){
            logexit("accept");
        }
        
        struct ClientData *cdata = malloc(sizeof(*cdata));
        if(!cdata){
            logexit("malloc");
        }
        memcpy(&(cdata->storage), &c_storage, sizeof(c_storage));
        memcpy(&(server_data->cdata), cdata, sizeof(*cdata));
        server_data->cdata->csock = csock;


        pthread_t tid;
        switch (server_data->server_type){
        case SE:
            pthread_create(&tid, NULL, SE_thread, server_data);
            break;
        
        case CII:
            pthread_create(&tid, NULL, CII_thread, server_data);
            break;
        }

    }

    close(s);
    exit(EXIT_SUCCESS);
}

