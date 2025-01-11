#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"

typedef struct sockaddr_storage sockaddr_storage;
typedef struct sockaddr sockaddr;

void usage(char **argv) {
    printf("Exemplo de uso:\n");
    printf("./client <ip> <user server port> <loc server port> <loc id>\n");
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
    if (argc < 5) {
        usage(argv);
    }

    uint16_t user_port, loc_port;
    user_port = format_port(argv[2]);
    loc_port = format_port(argv[3]);

    uint8_t loc_id = (uint8_t)atoi(argv[4]);
    if (loc_id < 1 || loc_id > 10) {
        printf("Invalid argument\n");
        return 1;
    }

    // handle user server connection
    sockaddr_storage user_storage;
    if (0 != addrparse(argv[1], user_port, &user_storage)) {
        logexit("erro no parse\n");
    }

    int usersock = socket(user_storage.ss_family, SOCK_STREAM, 0);
    if (usersock == -1) {
        logexit("erro no socket\n");
    }

    sockaddr *useraddr = (sockaddr *)(&user_storage);
    if (0 != connect(usersock, useraddr, sizeof(user_storage))) {
        logexit("erro na conexao\n");
    }

    char useraddrstr[MSGSIZE];
    addrtostr(useraddr, useraddrstr, MSGSIZE);

    printf("connected to SU: %s\n", useraddrstr);

    // handle loc server connection
    sockaddr_storage loc_storage;
    if (0 != addrparse(argv[1], loc_port, &loc_storage)) {
        logexit("erro no parse\n");
    }

    int locsock = socket(loc_storage.ss_family, SOCK_STREAM, 0);
    if (locsock == -1) {
        logexit("erro no socket\n");
    }

    sockaddr *locaddr = (sockaddr *)(&loc_storage);
    if (0 != connect(locsock, locaddr, sizeof(loc_storage))) {
        logexit("erro na conexao\n");
    }

    char locaddrstr[MSGSIZE];
    addrtostr(locaddr, locaddrstr, MSGSIZE);

    printf("connected to SL: %s\n", locaddrstr);

    char buffer[MSGSIZE];
    while (1) {
        memset(buffer, 0, MSGSIZE);
        printf("mensagem ('kill' para encerrar)> ");
        fgets(buffer, MSGSIZE - 1, stdin);

        size_t count = send(usersock, buffer, strlen(buffer) + 1, 0);
        if (count != strlen(buffer) + 1) {
            logexit("erro no send\n");
        }

        if (strncmp(buffer, "kill", 4) == 0) {
            printf("encerrando conexão...\n");
            break;
        }

        memset(buffer, 0, MSGSIZE);

        count = recv(usersock, buffer, MSGSIZE, 0);
        if (count == 0) {
            break;
        }
        puts(buffer);
    }

    exit(EXIT_SUCCESS);

    return 0;
}