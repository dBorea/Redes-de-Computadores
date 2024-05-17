#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <regex.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int C_id;

int build_command_msg(Message *cmdMsg){

    char inbuf[BUFSZ];
    while(cmdMsg->type == -1){
        memset(inbuf, 0, BUFSZ);
        fgets(inbuf, BUFSZ-1, stdin);

        if(strcmp(inbuf, "kill\n") == 0){
            char killBuf[10];
            sprintf(killBuf, "%d\n", C_id);
            changeMessage(cmdMsg, REQ_REM, killBuf);
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
        return BOTH_SERVERS;
    
    case REQ_INFOSE:
    case REQ_STATUS:
        if(0 != sendMessage(socketSE, msg_out)){
            logexit("Send SE\n");
        }
        return SE;

    case REQ_INFOSCII:
    case REQ_UP:    
    case REQ_NONE:
    case REQ_DOWN:
        if(0 != sendMessage(socketSCII, msg_out)){
            logexit("Send SCII\n");
        }
        return CII;
    }
    
}

int handle_received_messages(Message* msg_in){
    int old_value, new_value;

    switch(msg_in->type){
        case RES_ADD:
            C_id = atoi(msg_in->payloadstr);
            printf("Servidor SE New ID: %d\n", C_id);
            printf("Servidor SCII New ID: %d\n", C_id);
            break;

        case RES_INFOSE:
            printf("producao atual: %s", msg_in->payloadstr);
            break;

        case RES_INFOSCII:
            printf("consumo atual: %s", msg_in->payloadstr);
            break;

        case RES_STATUS:
            printf("estado atual: %s", msg_in->payloadstr);
            if(strcmp(msg_in->payloadstr, "alta\n") == 0) return 5;
            if(strcmp(msg_in->payloadstr, "moderada\n") == 0) return 4;
            if(strcmp(msg_in->payloadstr, "baixa\n") == 0) return 3;
            break;

        case RES_UP:
        case RES_DOWN:
            sscanf(msg_in->payloadstr, "%d %d", &old_value, &new_value);
            printf("consumo antigo: %d\nconsumo atual: %d\n", old_value, new_value);
            break;

        case RES_NONE:
            old_value = atoi(msg_in->payloadstr);
            printf("consumo antigo: %d\n", old_value);
            break;

        case ERROR:
            if(atoi(msg_in->payloadstr) == 1) return -1;
            if(atoi(msg_in->payloadstr) == 2) return -2;
            break;

        case OK:
            if(atoi(msg_in->payloadstr) == 1) return 1;
            break;
    }

    return 0;
}

int handle_incoming_messages(int sentTo, int socketSE, int socketSCII, int msgOutType){
    Message* msg_in_SE = buildMessage(-1, "");
    Message* msg_in_SCII = buildMessage(-1, "");
    Message* msg_out_buff = buildMessage(-1, "");
    int bitcount;

    switch(sentTo){
        case SE:
            if(0 != getMessage(socketSE, msg_in_SE, &bitcount)){
                logexit("SE server stopped responding\n");
            }
            break;
        
        case CII:
            if(0 != getMessage(socketSCII, msg_in_SCII, &bitcount)){
                logexit("SCII server stopped responding\n");
            }
            break;

        case BOTH_SERVERS:
            if(0 != getMessage(socketSE, msg_in_SE, &bitcount) || 0 != getMessage(socketSCII, msg_in_SCII, &bitcount)){
                logexit("At least one server stopped responding\n");
            }
            break;
    }

    switch(msgOutType){
        case REQ_INFOSE:
            handle_received_messages(msg_in_SE);
            break;

        case REQ_STATUS:
            switch(handle_received_messages(msg_in_SE)){
                case 3:
                    changeMessage(msg_out_buff, REQ_DOWN, "");
                    break;
                case 4:
                    changeMessage(msg_out_buff, REQ_NONE, "");
                    break;
                case 5:
                    changeMessage(msg_out_buff, REQ_UP, "");
                    break;
                default:
                    logexit("REQ_STATUS returned invalid response from server\n");
            }
            sendMessage(socketSCII, msg_out_buff);
            return msg_out_buff->type;

        case REQ_INFOSCII:
        case REQ_DOWN:
        case REQ_NONE:
        case REQ_UP:
            handle_received_messages(msg_in_SCII);
            break;

        case REQ_REM:
            if(handle_received_messages(msg_in_SE) == 1 && handle_received_messages(msg_in_SCII) == 1){
                printf("Successful disconnect\n");
                return 1;
            } 
            else if(handle_received_messages(msg_in_SE) == -2 && handle_received_messages(msg_in_SCII) == -2){
                printf("Client not found\n");
                return -2;
            }
            else logexit("REQ_REM returned invalid messages\n");
    }

    return 0;
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
    int running = 1;

    if(0 != sendMessage(sockSE, msg_out) || 0 != sendMessage(sockSCII, msg_out)){
        logexit("Add request failed");
    }

    if(0 != getMessage(sockSE, msg_in, &bitcount) || 0 != getMessage(sockSCII, msg_in, &bitcount)){
        logexit("At least one server stopped responding\n");
    }
    
    if (handle_received_messages(msg_in) == -1){
        running = 0;
        printf("Client limit exceeded\n");
    }

    while(running){
        // LOOP DE EXECUÇÃO INTERNO
        changeMessage(msg_out, -1, "");
        build_command_msg(msg_out);

        int sentTo = handle_and_send_msgOut(sockSE, sockSCII, msg_out);

        int retCode = handle_incoming_messages(sentTo, sockSE, sockSCII, msg_out->type);

        // Espera por mensagens mais uma vez em caso de requisições multi-step
        if(retCode == REQ_UP || retCode == REQ_NONE || retCode == REQ_DOWN){
            handle_incoming_messages(CII, sockSE, sockSCII, retCode);
        }


    } 

    close(sockSE);
    close(sockSCII);

    exit(EXIT_SUCCESS);    
}