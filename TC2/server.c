#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <bits/pthreadtypes.h>
#include <asm-generic/socket.h>

enum ServerType {
    SE,
    CII
};

struct client_data{
    int csock;
    struct sockaddr_storage storage;
};

void * SE_thread(void *data){
    struct client_data *cdata = (struct client_data *)data;
    struct sockaddr *caddr = (struct sockaddr *)(&cdata->storage);

    char caddrstr[BUFSZ];
    addrtostr(caddr, caddrstr, BUFSZ);
    printf("[SE] [log] connection from %s\n", caddrstr);

    while(1){ // Loop interno do server SE, gerencia as mensagens e a comunicação com o cliente
        // EXECUTION LOOP
    }

    pthread_exit(EXIT_SUCCESS);
}

void * CII_thread(void *data){
    struct client_data *cdata = (struct client_data *)data;
    struct sockaddr *caddr = (struct sockaddr *)(&cdata->storage);

    char caddrstr[BUFSZ];
    addrtostr(caddr, caddrstr, BUFSZ);
    printf("[CII] [log] connection from %s\n", caddrstr);

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
    if(argc < 3){
        usage(argc, argv);
    }

    struct sockaddr_storage storage;
    if(0 != server_sockaddr_init(argv[1], argv[2], &storage)){
        usage(argc, argv);
    }

    int server_type = -1;
    if(strcmp(argv[2], "12345") == 0){
        server_type = SE;
        printf("Server type [SE] ");
    } else if (strcmp(argv[2], "54321") == 0){
        printf("Server type [CII] ");
        server_type = CII;
    } else {
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
        
        struct client_data *cdata = malloc(sizeof(*cdata));
        if(!cdata){
            logexit("malloc");
        }
        memcpy(&(cdata->storage), &c_storage, sizeof(c_storage));

        pthread_t tid;
        switch (server_type){
        case SE:
            pthread_create(&tid, NULL, SE_thread, cdata);
            break;
        
        case CII:
            pthread_create(&tid, NULL, CII_thread, cdata);
            break;
        }

    }

    close(s);
    exit(EXIT_SUCCESS);
}

