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

int main(int argc, char **argv) {
    if (argc < 3) {
        logexit("numero de parametros insuficiente\n");
    }

    sockaddr_storage storage;
    if (0 != addrparse(argv[1], argv[2], &storage)) {
        logexit("erro no parse\n");
    }

    int s = socket(storage.ss_family, SOCK_STREAM, 0);
    if (s == -1) {
        logexit("erro no socket\n");
    }

    sockaddr *addr = (sockaddr *)(&storage);
    if (0 != connect(s, addr, sizeof(storage))) {
        logexit("erro na conexao\n");
    }

    char addrstr[MSGSIZE];
    addrtostr(addr, addrstr, MSGSIZE);

    printf("connected to %s\n", addrstr);

    char buffer[MSGSIZE];
    memset(buffer, 0, MSGSIZE);
    printf("mensagem> ");
    fgets(buffer, MSGSIZE - 1, stdin);

    size_t count = send(s, buffer, strlen(buffer) + 1, 0);
    if (count != strlen(buffer) + 1) {
        logexit("erro no send\n");
    }

    memset(buffer, 0, MSGSIZE);
    unsigned total = 0;
    while (1) {
        count = recv(s, buffer + total, MSGSIZE - total, 0);
        if (count == 0) {
            break;
        }

        total += count;
    }

    close(s);

    printf("received %u bytes\n", total);
    puts(buffer);

    exit(EXIT_SUCCESS);
}