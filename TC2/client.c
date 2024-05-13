#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <regex.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

bool fit_args(char *args, char *pattern){
    regex_t reg;
    regcomp(&reg, pattern, REG_EXTENDED);
    bool fit = regexec(&reg, args, 0, NULL, 0);
    regfree(&reg);
    return fit;
}

int handle_commands(Message *cmdMsg){

    char inbuf[BUFSZ];
    while(cmdMsg->type == -1){
        memset(inbuf, 0, BUFSZ);
        fgets(inbuf, BUFSZ-1, stdin);

        if(strcmp(inbuf, "kill\n") == 0){
            changeMessage(cmdMsg, REQ_REM, "0\n");
        } else
            if(strcmp(inbuf, "display info se\n") == 0){
                changeMessage(cmdMsg, REQ_INFOSE, "");
        } else
            if(strcmp(inbuf, "display info scii\n") == 0){
                changeMessage(cmdMsg, REQ_INFOSCII, "");
        } else
            if(strcmp(inbuf, "query condition\n") == 0){
                changeMessage(cmdMsg, REQ_STATUS, "");
        } 
    }

    return cmdMsg->type;
}

int handle_and_send_msgOut(int socketSE, int socketSCII, Message *msg_out){
    switch (msg_out->type){
    case REQ_ADD:
    case REQ_REM:
        if(0 != sendMessage(socketSE, msg_out) || 0 != sendMessage(socketSCII, msg_out)){
            logexit("Send both\n");
        }
        break;
    
    case REQ_INFOSE:
    case REQ_STATUS:
        if(0 != sendMessage(socketSE, msg_out)){
            logexit("Send SE\n");
        }
        break;

    case REQ_INFOSCII:
    case REQ_UP:    
    case REQ_NONE:
    case REQ_DOWN:
        if(0 != sendMessage(socketSCII, msg_out)){
            logexit("Send SE\n");
        }
        break;
    }
    
}

int handle_incoming_messages(int socketSE, int socketSCII, int type, Message *msg_in){

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

    int sockSE, sockSCII;
    if(strcmp(argv[2], "12345") == 0 && strcmp(argv[3], "54321") == 0){
        sockSE = socket(storage1.ss_family, SOCK_STREAM, 0);
        sockSCII = socket(storage2.ss_family, SOCK_STREAM, 0);
    }
    else if(strcmp(argv[2], "54321") == 0 && strcmp(argv[3], "12345") == 0){
        sockSE = socket(storage2.ss_family, SOCK_STREAM, 0);
        sockSCII = socket(storage1.ss_family, SOCK_STREAM, 0);
    } else {
        logexit("Only ports \'12345\' and \'54321\' are accepted.\n");
    }

    if(sockSE == -1 || sockSCII == -1){
        logexit("socket");
    }

    int enable = 1;
    if(0 != setsockopt(sockSE, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) || 
        0 != setsockopt(sockSCII, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))){
        logexit("setsockopt");
    }

    struct sockaddr *addr1 = (struct sockaddr *)(&storage1);
    struct sockaddr *addr2 = (struct sockaddr *)(&storage2);
    if(0 != connect(sockSE, addr1, sizeof(storage1)) || 0 != connect(sockSCII, addr2, sizeof(storage2))){
        logexit("connect");
    }

    char addrstr1[BUFSZ];
    char addrstr2[BUFSZ];
    addrtostr(addr1, addrstr1, BUFSZ);
    addrtostr(addr2, addrstr2, BUFSZ);
    printf("connected to %s and %s\n", addrstr1, addrstr2);

    Message *msg_out = buildMessage(REQ_ADD, "");
    Message *msg_in = buildMessage(-1, "");
    int bitcount;

    if(0 != sendMessage(sockSE, msg_out) || 0 != sendMessage(sockSCII, msg_out)){
        logexit("Add request failed");
    }
    // getMessage(sockSE, msg_in, &bitcount);
    // printf("[MSG] %s\n", getMsgAsStr(msg_in, NULL));

    while(1){
        // LOOP DE EXECUÇÃO INTERNO
        changeMessage(msg_out, -1, "");
        handle_commands(msg_out);

        handle_and_send_msgOut(sockSE, sockSCII, msg_out);

        //handle_incoming_messages(sockSE, sockSCII, msg_out->type, msg_in);


        
        
        //Message *msg_in = buildMessage(-1, "");


    } 

    close(sockSE);
    close(sockSCII);

    exit(EXIT_SUCCESS);    
}