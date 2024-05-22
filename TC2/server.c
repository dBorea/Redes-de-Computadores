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

static const char *ServerTypeStr[] = {
    "SE",
    "CII"
};

typedef struct ClientData{
    int csock;
    struct sockaddr_storage storage;
    int client_id;
} ClientData;

typedef struct ServerData{
    int clientIds[10];

    int server_type;
    int dataValue;

    ClientData *cdata;
} ServerData;

// Gera valor aleatório no range 0~100% -- type: 1(aumentar) 0(neutro) -1(diminuir)
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

int tryIncludingClient(ServerData *server_data, ClientData *cdata){
    int i=0;
    for(; i<10; i++){
        if(server_data->clientIds[i] == 0)
        break;
    } if(i == 10) return -1;

    printf("Client %d added\n", i+1);
    server_data->clientIds[i] = 1;

    cdata->client_id = i+1;
    return i+1;
}

int tryDisconnectingClient(ServerData *server_data, int C_id){
    if(server_data->clientIds[C_id - 1] != 1)
        return -1;

    server_data->clientIds[C_id - 1] = 0;
    printf("Servidor %s Client %d removed\n", ServerTypeStr[server_data->server_type], C_id);
    return 0;
}

int serverMsgHandler(ServerData *server_data, ClientData *cdata, Message *msg){ 
    int socket = cdata->csock;
    Message* bufMsg = buildMessage(-1, "");
    char strBuf[BUFSZ];
    memset(strBuf, 0, BUFSZ);

    switch(msg->type){
        case REQ_ADD:
            int C_id = tryIncludingClient(server_data, cdata);
            if(C_id == -1){
                changeMessage(bufMsg, ERROR, "01\n");
                sendMessage(socket, bufMsg, true);
                return -1;
            }
            else {
                sprintf(strBuf, "%d\n", C_id);
                changeMessage(bufMsg, RES_ADD, strBuf);
                sendMessage(socket, bufMsg, true);
            }
            break;
        
        case REQ_REM:
            if(0 != tryDisconnectingClient(server_data, atoi(msg->payloadstr))){
                changeMessage(bufMsg, ERROR, "02\n");
                sendMessage(socket, bufMsg, true);
            } else {
                changeMessage(bufMsg, OK, "01\n");
                sendMessage(socket, bufMsg, true);
                return -1;
            }
            break;

        case REQ_INFOSE:
            sprintf(strBuf, "%d kWh\n", server_data->dataValue);
            changeMessage(bufMsg, RES_INFOSE, strBuf);
            sendMessage(socket, bufMsg, true);
            break;

        case REQ_STATUS:
            if(server_data->dataValue <= 30){
                sprintf(strBuf, "baixa\n");
            } else if(server_data->dataValue <= 40){
                sprintf(strBuf, "moderada\n");
            } else {
                sprintf(strBuf, "alta\n");
            }
            changeMessage(bufMsg, RES_STATUS, strBuf);
            sendMessage(socket, bufMsg, true);
            server_data->dataValue = randomPowerGen();
            break;

        case REQ_INFOSCII:
            sprintf(strBuf, "%d%%\n", server_data->dataValue);
            changeMessage(bufMsg, RES_INFOSCII, strBuf);
            sendMessage(socket, bufMsg, true);
            break;

        case REQ_UP:
            int old_value_up = server_data->dataValue;
            server_data->dataValue = randomPowerUsage(1, server_data);
            sprintf(strBuf, "%d %d\n", old_value_up, server_data->dataValue);
            changeMessage(bufMsg, RES_UP, strBuf);
            sendMessage(socket, bufMsg, true);
            break;

        case REQ_NONE:
            sprintf(strBuf, "%d\n", server_data->dataValue);
            changeMessage(bufMsg, RES_NONE, strBuf);
            sendMessage(socket, bufMsg, true);
            break;

        case REQ_DOWN:
            int old_value_down = server_data->dataValue;
            server_data->dataValue = randomPowerUsage(-1, server_data);
            sprintf(strBuf, "%d %d\n", old_value_down, server_data->dataValue);
            changeMessage(bufMsg, RES_DOWN, strBuf);
            sendMessage(socket, bufMsg, true);
            break;
    }
    return 0;
}

void * client_thread(void *data){
    ServerData *server_data = (struct ServerData *)data;
    struct ClientData *cdata = malloc(sizeof(ClientData));
    memcpy(cdata, (struct ClientData *)(&server_data->cdata), sizeof(ClientData));

    // struct sockaddr *caddr = (struct sockaddr *)(&cdata->storage);
    // char caddrstr[BUFSZ];
    // addrtostr(caddr, caddrstr, BUFSZ);
    // printf("[SE] [log] connection from %s\n", caddrstr);

    Message *msg_in = buildMessage(-1, "");  

    while(1){ // Loop interno do server SE, gerencia as mensagens e a comunicação com o cliente
        // EXECUTION LOOP 
        int bitcount;
        if(0 != getMessage(cdata->csock, msg_in, &bitcount, true)){
            // printf("[log] client forcefully disconnected\n");
            tryDisconnectingClient(server_data, cdata->client_id);
            close(cdata->csock);
            break;
        }
        // printf("[msg] (%d bytes) %s",(int)bitcount, getMsgAsStr(msg_in, NULL));
        fflush(stdout);

        if(-1 == serverMsgHandler(server_data, cdata, msg_in)){
            break;
        }
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
        // printf("Server type [SE]\n");
        server_data->server_type = SE;
        server_data->dataValue = randomPowerGen();
        // printf("[SE] [log] Power Gen. initialized as %d\n", server_data->dataValue);

    } 
    else if (strcmp(argv[2], "54321") == 0){
        // printf("Server type [CII]\n");
        server_data->server_type = CII;
        server_data->dataValue = randomPowerUsage(0, server_data);
        // printf("[CII] [log] Power Usage. initialized as %d\n", server_data->dataValue);

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
    // printf("bound to %s, waiting connection\n", addrstr);
    printf("Starting to listen..\n");

    struct sockaddr_storage c_storage;
    struct sockaddr *caddr = (struct sockaddr *)(&c_storage);
    socklen_t caddrlen = sizeof(c_storage);

    bool running = true;
    while(running){ // Loop externo, apenas para conexões;
        struct ClientData *cdata = malloc(sizeof(*cdata));
        if(!cdata){
            logexit("malloc");
        }
        
        cdata->csock = accept(s, caddr, &caddrlen);
        if(cdata->csock == -1){
            logexit("accept");
        }
        memcpy(&(cdata->storage), &c_storage, sizeof(c_storage));
        memcpy(&(server_data->cdata), cdata, sizeof(*cdata));

        pthread_t tid;
        pthread_create(&tid, NULL, client_thread, server_data);

    }

    close(s);
    exit(EXIT_SUCCESS);
}

