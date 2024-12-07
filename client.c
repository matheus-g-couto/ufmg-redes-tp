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

    sockaddr_storage user_storage, loc_storage;
    if (0 != addrparse(argv[1], user_port, &user_storage)) {
        logexit("erro no parse\n");
    }

    int s = socket(user_storage.ss_family, SOCK_STREAM, 0);
    if (s == -1) {
        logexit("erro no socket\n");
    }

    sockaddr *addr = (sockaddr *)(&user_storage);
    if (0 != connect(s, addr, sizeof(user_storage))) {
        logexit("erro na conexao\n");
    }

    char addrstr[MSGSIZE];
    addrtostr(addr, addrstr, MSGSIZE);

    printf("connected to %s\n", addrstr);

    char buffer[MSGSIZE];
    while (1) {
        memset(buffer, 0, MSGSIZE);
        printf("mensagem ('kill' para encerrar)> ");
        fgets(buffer, MSGSIZE - 1, stdin);

        size_t count = send(s, buffer, strlen(buffer) + 1, 0);
        if (count != strlen(buffer) + 1) {
            logexit("erro no send\n");
        }

        if (strncmp(buffer, "kill", 4) == 0) {
            printf("encerrando conexão...\n");
            break;
        }

        memset(buffer, 0, MSGSIZE);

        count = recv(s, buffer, MSGSIZE, 0);
        if (count == 0) {
            break;
        }
        puts(buffer);
    }

    exit(EXIT_SUCCESS);

    return 0;
}