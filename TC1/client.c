#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <regex.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

void usage(int argc, char **argv) {
    printf("usage: %s <server IP> <server port>\n", argv[0]);
    printf("example: %s 127.0.0.1 51511\n",argv[0]);
    exit(EXIT_FAILURE);
}

bool fit_args(char *args, char *pattern){
    regex_t reg;
    regcomp(&reg, pattern, REG_EXTENDED);
    bool fit = regexec(&reg, args, 0, NULL, 0);
    regfree(&reg);
    return fit;
}

int arg_check(char *args, int type){
    switch(type){
        case SOLO_ID:
            if(0 != fit_args(args, "^[0-7]\n$")){
                printf("sala inválida\n");
                return -1;
            }            
        break;

        case FULL_INFO:
            char parsebuf[BUFSZ];
            memset(parsebuf, 0, BUFSZ);
            strcpy(parsebuf, args);

            char *sala_id, *temp, *umid, *estados_vent;
            sala_id = strtok(parsebuf, " ");
            temp = strtok(NULL, " ");
            umid = strtok(NULL, " ");
            estados_vent = strtok(NULL, "\0");

            if(0 != fit_args(sala_id, "^[0-7]$")){
                printf("sala inválida\n");
                return -1;
            } else if(0 != fit_args(temp, "^(0|[1-3]?[0-9]|40)$") || 
                      0 != fit_args(umid, "^0|[1-9][0-9]?|100$") ||
                      0 != fit_args(estados_vent, "^(1[0-2] 2[0-2] 3[0-2] 4[0-2])+\n$")){
                printf("sensores inválidos\n");
                return -1;
            }
        break;
    }
    return 0;
}

int file_check(char* filename, char* arg_string_out){
    filename[strcspn(filename, "\n")] = 0;
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("sala inválida\n");
        return -1;
    }

    int args[8] = {-1, -1, -1, -1, -1, -1, -1, -1} ;
    int toomanyargs = 0;
    for(int i=0; !feof(file) && i<8; i++){
        fscanf(file, "%d", &args[i]);
        //printf("[file] scanned: %d\n", args[i]);
    }

    if(args[7] != -1) { 
        //printf("too many args\n"); 
        toomanyargs = 1; 
    }

    if(args[0] < 0 || args[0] > 7){
        printf("sala inválida\n");
        return -1;
    } else if(args[1] < 0 || args[1] > 40 ||
              args[2] < 0 || args[2] > 100 ||
              args[3] < 10 || args[3] > 12 ||
              args[4] < 20 || args[4] > 22 ||
              args[5] < 30 || args[5] > 32 ||
              args[6] < 40 || args[6] > 42 || toomanyargs == 1){
                //printf("sensores inválidos\n");
                return -1;
              }
    sprintf(arg_string_out, "%d %d %d %d %d %d %d\n", args[0], args[1], args[2], args[3], args[4], args[5], args[6]);
    //printf("[file] read: %s", arg_string_out);
    return 0;
}

int handle_response(Message *msg){
    if(msg->type == kill){
        return -1;
    } else if(msg->type == ERROR){
        if(0 == strcmp(msg->payloadstr, "ERROR 02\n")){
            printf("sala já existe\n");
        } else if(0 == strcmp(msg->payloadstr, "ERROR 03\n")){
            printf("sala inexistente\n");
        } else if(0 == strcmp(msg->payloadstr, "ERROR 05\n")){
            printf("sensores já instalados\n");
        } else if(0 == strcmp(msg->payloadstr, "ERROR 06\n")){
            printf("sensores não instalados\n");
        }
    } else if(msg->type == OK){
        if(0 == strcmp(msg->payloadstr, "OK 01\n")){
            printf("sala instanciada com sucesso\n");
        } else if(0 == strcmp(msg->payloadstr, "OK 02\n")){
            printf("sensores inicializados com sucesso\n");
        } else if(0 == strcmp(msg->payloadstr, "OK 03\n")){
            printf("sensores desligados com sucesso\n");
        } else if(0 == strcmp(msg->payloadstr, "OK 04\n")){
            printf("informações atualizadas com sucesso\n");
        }
    } else printf("%s", msg->payloadstr);
    return 0;
}

Message* get_command(){
    Message *cmd = malloc(sizeof(Message));
    memset(cmd->payloadstr, 0, MSGSZ);
    cmd->type = -1;

    char inbuf[BUFSZ];
    char filebuf[BUFSZ];
    while(cmd->type == -1){
        memset(inbuf, 0, BUFSZ);
        fgets(inbuf, BUFSZ-1, stdin);
        
        if(StartsWith(inbuf, "register ")){ // Pedido de registro de sala // ARGS: sala_id
            strremove(inbuf, "register ");
            if(0 == arg_check(inbuf, SOLO_ID)){
                strcpy(cmd->payloadstr, inbuf);
                cmd->type = CAD_REQ;
            }
        } else 
            if(StartsWith(inbuf, "init file ")){
                strremove(inbuf, "init file ");
                if(0 == file_check(inbuf, filebuf)){
                    strcpy(cmd->payloadstr, filebuf);
                    cmd->type = INI_REQ;
                }
        } else 
            if(StartsWith(inbuf, "init info ")){
                strremove(inbuf, "init info ");
                if(0 == arg_check(inbuf, FULL_INFO)){
                    strcpy(cmd->payloadstr, inbuf);
                    cmd->type = INI_REQ;
                }
        } else 
            if(StartsWith(inbuf, "shutdown ")){ // Pedido de desligação de sensores da sala // ARGS: sala_id
                strremove(inbuf, "shutdown ");
                //if(0 == arg_check(inbuf, SOLO_ID)){
                    strcpy(cmd->payloadstr, inbuf);
                    cmd->type = DES_REQ;
                //}
        } else 
            if(StartsWith(inbuf, "update file ")){
                strremove(inbuf, "update file ");
                if(0 == file_check(inbuf, filebuf)){
                    strcpy(cmd->payloadstr, filebuf);
                    cmd->type = ALT_REQ;
                }
        } else 
            if(StartsWith(inbuf, "update info ")){ // Pedido de alteração dos valores da sala // ARGS: sala_id, temp, umid, estados_vent
                strremove(inbuf, "update info ");
                if(0 == arg_check(inbuf, FULL_INFO)){
                    strcpy(cmd->payloadstr, inbuf);
                    cmd->type = ALT_REQ;
                }
        } else 
            if(StartsWith(inbuf, "load info ")){ // Pedido de informações da sala // ARGS: sala_id
                strremove(inbuf, "load info ");
                //if(0 == arg_check(inbuf, SOLO_ID)){
                    strcpy(cmd->payloadstr, inbuf);
                    cmd->type = SAL_REQ;
                //}
        } else 
            if(StartsWith(inbuf, "load rooms")){ // Pedido de informações de todas as salas // AGRS: n/a
                cmd->type = INF_REQ;
                strcpy(cmd->payloadstr, "\n");
                //strcpy(cmd->payloadstr, "full info request\n");
        } else
            if(StartsWith(inbuf, "kill")){ // Desliga o servidor // AGRS: n/a
                cmd->type = kill;
                //strcpy(cmd->payloadstr, "\n");
                //strcpy(cmd->payloadstr, "shutdown request\n");
        } else {
            // cmd->type = EXIT;
            // strcpy(cmd->payloadstr, "unkwnown command\n");
            return NULL;
        }
    }

    return cmd;
}

int main(int argc, char **argv) {
    if(argc < 3){
        usage(argc, argv);
    }


    struct sockaddr_storage storage;
    if(0 != addrparse(argv[1], argv[2], &storage)){
        usage(argc, argv);
    }

    int s;
    s = socket(storage.ss_family, SOCK_STREAM, 0);
    if(s == -1){
        logexit("socket");
    }

    int enable = 1;
    if(0 != setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))){
        logexit("setsockopt");
    }

    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if(0 != connect(s, addr, sizeof(storage))){
        logexit("connect");
    }

    //char addrstr[BUFSZ];
    //addrtostr(addr, addrstr, BUFSZ);
    //printf("connected to %s\n", addrstr);

    while(1){
        Message *msg_out = get_command();
        if(msg_out == NULL){
            break;
        }

        if(0 != send_message(s, msg_out)){
            logexit("send");
        }
        if(msg_out->type == EXIT || msg_out->type == kill){
            free(msg_out);
            break;
        }
        free(msg_out);

        Message *msg_in = malloc(sizeof(Message));
        int count;
        if(0 != get_message(s, msg_in, &count)){
            printf("[log] server unresponsive\n");
            continue;
        }
        
        if(0 != handle_response(msg_in)){
            break;
        }
    } 

    close(s);

    exit(EXIT_SUCCESS);    
}