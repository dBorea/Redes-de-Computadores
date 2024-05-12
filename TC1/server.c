#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>

#define BUFSZ 1024

enum Arguments {
    ROOM_ID,
    TEMP,
    HUMID,
    FAN_1,
    FAN_2,
    FAN_3,
    FAN_4
};

typedef struct Room{
    int room_state;
    int temperature;
    int humidity;
    int fan_states[4];
}Room;

void usage(int argc, char **argv) {
    printf("usage: %s <v4/v6> <server port>\n", argv[0]);
    printf("example: %s v4 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

Room* newRoom(){
    Room *r = malloc(sizeof(Room));
    r->room_state = -1;
    r->humidity = -1;
    r->temperature = -1;
    for(int i=0; i<4; i++){
        r->fan_states[i] = -1;
    }
    return r;
}

void print_room(Room **rooms_database, int *args){
    printf("[log] sala %d: [state] %d [temp] %d [umid] %d [fans] %d %d %d %d\n",
                                                                args[ROOM_ID],
                                                                rooms_database[args[ROOM_ID]]->room_state,
                                                                rooms_database[args[ROOM_ID]]->temperature,
                                                                rooms_database[args[ROOM_ID]]->humidity,
                                                                rooms_database[args[ROOM_ID]]->fan_states[0],
                                                                rooms_database[args[ROOM_ID]]->fan_states[1],
                                                                rooms_database[args[ROOM_ID]]->fan_states[2],
                                                                rooms_database[args[ROOM_ID]]->fan_states[3]); 
}

void cleanRoom(Room *r){
    r->room_state = -1;
    r->humidity = -1;
    r->temperature = -1;
    for(int i=0; i<4; i++){
        r->fan_states[i] = -1;
    }
}

void updateSensors(Room **rooms_database, int *args){
    rooms_database[args[ROOM_ID]]->humidity = args[HUMID];
    rooms_database[args[ROOM_ID]]->temperature = args[TEMP];
    for(int i=0; i<4; i++){
        rooms_database[args[ROOM_ID]]->fan_states[i] = args[FAN_1 + i]%10;
    }
}

void parse_req_args(Message *msg, int *args){
    if((msg->type == CAD_REQ || msg->type == DES_REQ || msg->type == SAL_REQ)){ // Caso a msg tenha 1 arg
        args[0] = atoi(msg->payloadstr);
        //printf("[log] converted %s to %d\n", msg->payloadstr, args[0]);
    } else 
    if ((msg->type == INI_REQ || msg->type == ALT_REQ)){ // Caso a msg tenha todos os args
        char buf[BUFSZ];
        strcpy(buf, msg->payloadstr);
        char *token = strtok(buf, " ");
        for(int i=0; i<7; i++){
            args[i]= atoi(token);
            //printf("[log] arg %d: %d\n", i, args[i]);
            token = strtok(NULL, " ");
        }
    }
}

int check_for_room(Room** room_database, int inputid){
    //printf("[log] estado da sala %d: %d\n", inputid, room_database[inputid]->room_state);
    if(inputid < 0 || inputid > 7) return 0;
    if(room_database[inputid]->room_state == -1) return 0;
    return 1;
}

int check_sensors(Room** room_database, int inputid){
    for(int i=0; i<4; i++){
        if(room_database[inputid]->fan_states[i] == -1) return 0;
    }
    if(room_database[inputid]->temperature == -1 || room_database[inputid]->humidity == -1) return 0;
    return 1;
}

void build_and_send(int ret_type, int ret_code, int socket){
    Message *msg = malloc(sizeof(Message));
    msg->type = ret_type;
    switch(ret_type){ // TEMPORÁRIO, MUDAR DEPOIS
        case ERROR:
            printf("[ERRO] código %d\n", ret_code);
            sprintf(msg->payloadstr,"0%d\n", ret_code);
            send_message(socket, msg);
            break;
        case OK:
            printf("[OK] código %d\n", ret_code);
            sprintf(msg->payloadstr,"0%d\n", ret_code);
            send_message(socket, msg);
            break;
    }
}

void send_sal_res(Room **room_database, int *args, int socket){
    Message *msg = malloc(sizeof(Message));
    Room *r = room_database[args[ROOM_ID]];
    msg->type = SAL_RES;
    sprintf(msg->payloadstr, "sala %d: %d %d 1%d 2%d 3%d 4%d\n", args[ROOM_ID], r->temperature, r->humidity, 
                                                                r->fan_states[0], r->fan_states[1], r->fan_states[2], r->fan_states[3]);
    if(0 != send_message(socket, msg)){
        logexit("send");
    }
}

void send_inf_res(Room **room_database, int *args, int socket, bool *isthereroom){
    Message *msg = malloc(sizeof(Message));
    msg->type = INF_RES;

    strcpy(msg->payloadstr, "salas:");
    char tempstr[100];
    for(int i=0; i<8; i++){
        memset(tempstr, 0, 100);
        if(isthereroom[i]){
            if(room_database[i]->room_state == 0){
                sprintf(tempstr," %d (%d %d %d %d %d %d)", i, room_database[i]->temperature, room_database[i]->humidity, 
                                                            room_database[i]->fan_states[0], room_database[i]->fan_states[1], 
                                                            room_database[i]->fan_states[2], room_database[i]->fan_states[3]);
            } else {
                sprintf(tempstr," %d (%d %d 1%d 2%d 3%d 4%d)", i, room_database[i]->temperature, room_database[i]->humidity, 
                                                                room_database[i]->fan_states[0], room_database[i]->fan_states[1], 
                                                                room_database[i]->fan_states[2], room_database[i]->fan_states[3]);
            }
            strcat(msg->payloadstr, tempstr);
            
        }
    }
    strcat(msg->payloadstr, "\n");
    if(0 != send_message(socket, msg)){
        logexit("send");
    }
}

int msg_handler(Room** room_database, int socket, Message *msg){
    int args[7];
    parse_req_args(msg, args);
    
    switch(msg->type){
        case kill:
            printf("[log] kill requested, shutting down\n");
            build_and_send(kill, -1, socket);
            close(socket);
            return -2;

        case EXIT:
            printf("[log] client disconnected\n");
            close(socket);
            return -1;

        case CAD_REQ: // registro de sala
            if(1 == check_for_room(room_database, args[ROOM_ID])){ // Sala JÁ existe
                build_and_send(ERROR, 2, socket);
            } else { // Sala não existe, é registrada
                room_database[args[ROOM_ID]]->room_state = 0;
                build_and_send(OK, 1, socket);
            }
            break;

        case INI_REQ: // iniciar sensores da sala
            if(0 == check_for_room(room_database, args[ROOM_ID])){ // Sala NÃO existe
                build_and_send(ERROR, 3, socket);
            } else if (1 == check_sensors(room_database, args[ROOM_ID])){ // Sensores já instalados
                build_and_send(ERROR, 5, socket);
            } else { // Sensores não instalados, são registrados
                updateSensors(room_database, args);
                room_database[args[ROOM_ID]]->room_state = 1;
                build_and_send(OK, 2, socket);
            }
            break;

        case DES_REQ: // desligar sensores da sala
            if(0 == check_for_room(room_database, args[ROOM_ID])){ // Sala NÃO existe
                build_and_send(ERROR, 3, socket);
            } else if (0 == check_sensors(room_database, args[ROOM_ID])){ // Sensores NÃO instalados
                build_and_send(ERROR, 6, socket);
            } else { // Sensores instalados, são desligados
                cleanRoom(room_database[args[ROOM_ID]]);
                room_database[args[ROOM_ID]]->room_state = 0;
                build_and_send(OK, 3, socket);
            }
            break;

        case ALT_REQ: // alterar info dos sensores
            if(0 == check_for_room(room_database, args[ROOM_ID])){ // Sala NÃO existe
                build_and_send(ERROR, 3, socket);
            } else if (0 == check_sensors(room_database, args[ROOM_ID])){ // Sensores NÃO instalados
                build_and_send(ERROR, 6, socket);
            } else { // Sensores instalados, são atualizados
                updateSensors(room_database, args);
                build_and_send(OK, 4, socket);
            }
            break;

        case SAL_REQ: // devolver info da sala
            if(0 == check_for_room(room_database, args[ROOM_ID])){ // Sala NÃO existe
                build_and_send(ERROR, 3, socket);
                break;
            } else if (0 == check_sensors(room_database, args[ROOM_ID])){ // Sensores NÃO instalados
                build_and_send(ERROR, 6, socket);
                break;
            } else { // Envia dados da sala
                send_sal_res(room_database, args, socket);
            }
            break;

        case INF_REQ: // devolver info de todas as salas
            bool thereisroom[8] = {false};
            bool empty = true;
            for(int i=0; i<8; i++){
                if(1 == check_for_room(room_database, i)) { thereisroom[i] = true; empty = false;}
            }

            if(empty){
                build_and_send(ERROR, 3, socket);
            } else{
                send_inf_res(room_database, args, socket, thereisroom);
            }
            break;
    }
    return 0;
}

int main(int argc, char **argv) {
    if(argc < 3){
        usage(argc, argv);
    }

    struct sockaddr_storage storage;
    if(0 != server_sockaddr_init(argv[1], argv[2], &storage)){
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
    if(0 != bind(s, addr, sizeof(storage))){
        logexit("bind");
    }

    if(0 != listen(s, 1)){
        logexit("listen");
    }

    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ);
    printf("bound to %s, waiting connection\n", addrstr);

    Room **room_database = malloc(sizeof(Room)*8);
    for(int i=0; i<8; i++){
        room_database[i] = newRoom();
    }

    bool running = true;

    while(running){ // Loop externo, apenas para conexões;
        struct sockaddr_storage c_storage;
        struct sockaddr *caddr = (struct sockaddr *)(&c_storage);
        socklen_t caddrlen = sizeof(c_storage);

        int csock = accept(s, caddr, &caddrlen);
        if(csock == -1){
            logexit("accept");
        }

        char caddrstr[BUFSZ];
        addrtostr(caddr, caddrstr, BUFSZ);
        printf("[log] client connected\n");

        while(running){ // Loop interno, gerencia as mensagens e a comunicação com o cliente
            Message *msg = malloc(sizeof *msg);
            int count;
            if(0 != get_message(csock, msg, &count)){
                printf("[log] client forcefully disconnected\n");
                close(csock);
                break;
            }
            printf("[msg] (%d bytes) %s %s",(int)count, stringFromType(msg->type), msg->payloadstr);
            fflush(stdout);

            int return_code = msg_handler(room_database, csock, msg);
            if (return_code == -2){
                running = false;
            } else if(0 != return_code){
                break;
            }
        }

/*
        sprintf(buf, "remote endpoint: %.1000s\n", caddrstr);
        count = send(csock, buf, strlen(buf)+1, 0);
        if(count != strlen(buf)+1){
            logexit("send");
        }
*/        
    }

    close(s);
    exit(EXIT_SUCCESS);
}