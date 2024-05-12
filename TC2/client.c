#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <regex.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

void handle_commands(){

}




void usage(int argc, char **argv) {
    printf("usage: %s <server IP> <server port 1> <server port 2>\n", argv[0]);
    printf("example: %s 127.0.0.1 12345 54321\n",argv[0]);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
    if(argc < 4){
        usage(argc, argv);
    }

    struct sockaddr_storage storage1;
    if(0 != addrparse(argv[1], argv[2], &storage1)){
        usage(argc, argv);
    }

    struct sockaddr_storage storage2;
    if(0 != addrparse(argv[1], argv[3], &storage2)){
        usage(argc, argv);
    }


    int sock1;
    sock1 = socket(storage1.ss_family, SOCK_STREAM, 0);
    int sock2;
    sock2 = socket(storage2.ss_family, SOCK_STREAM, 0);

    if(sock1 == -1 || sock2 == -1){
        logexit("socket");
    }

    int enable = 1;
    if(0 != setsockopt(sock1, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) || 0 != setsockopt(sock2, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))){
        logexit("setsockopt");
    }

    struct sockaddr *addr1 = (struct sockaddr *)(&storage1);
    struct sockaddr *addr2 = (struct sockaddr *)(&storage2);
    if(0 != connect(sock1, addr1, sizeof(storage1)) || 0 != connect(sock2, addr2, sizeof(storage2))){
        logexit("connect");
    }

    char addrstr1[BUFSZ];
    char addrstr2[BUFSZ];
    addrtostr(addr1, addrstr1, BUFSZ);
    addrtostr(addr2, addrstr2, BUFSZ);
    printf("connected to %s and %s\n", addrstr1, addrstr2);

    while(1){
        // EXECUTION LOOP
    } 

    close(sock1);
    close(sock2);

    exit(EXIT_SUCCESS);    
}