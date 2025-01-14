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

int start_server_sock(const char *ip, uint16_t port) {
    sockaddr_storage storage;
    if (0 != addrparse(ip, port, &storage)) {
        logexit("erro no parse\n");
    }

    int sock = socket(storage.ss_family, SOCK_STREAM, 0);
    if (sock == -1) {
        logexit("erro no socket\n");
    }

    sockaddr *addr = (sockaddr *)(&storage);
    if (0 != connect(sock, addr, sizeof(storage))) {
        logexit("erro na conexao\n");
    }

    return sock;
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
    int usersock = start_server_sock(argv[1], user_port);
    int locsock = start_server_sock(argv[1], loc_port);

    char buffer[MSGSIZE];

    int suid, slid;

    memset(buffer, 0, MSGSIZE);
    sprintf(buffer, "%d", loc_id);
    send(usersock, buffer, strlen(buffer) + 1, 0);

    memset(buffer, 0, MSGSIZE);
    recv(usersock, buffer, MSGSIZE, 0);
    suid = atoi(buffer);

    memset(buffer, 0, MSGSIZE);
    sprintf(buffer, "%d", loc_id);
    send(locsock, buffer, strlen(buffer) + 1, 0);

    memset(buffer, 0, MSGSIZE);
    recv(locsock, buffer, MSGSIZE, 0);
    slid = atoi(buffer);

    printf("SU New ID: %d\nSL New ID: %d\n", suid, slid);

    while (1) {
        memset(buffer, 0, MSGSIZE);
        fgets(buffer, MSGSIZE - 1, stdin);

        if (strncmp(buffer, "kill", 4) == 0) {
            send(usersock, buffer, strlen(buffer) + 1, 0);
            send(locsock, buffer, strlen(buffer) + 1, 0);
            printf("SU Successful disconnect\nSL Successful disconnect\n");
            break;
        }

        size_t count;
        if (0 == strncmp(buffer, "add", 3) || (0 == strncmp(buffer, "in", 2) && 0 != strncmp(buffer, "inspect", 7)) ||
            0 == strncmp(buffer, "out", 3)) {
            count = send(usersock, buffer, strlen(buffer) + 1, 0);
            if (count != strlen(buffer) + 1) {
                logexit("erro no send\n");
            }
            memset(buffer, 0, MSGSIZE);

            count = recv(usersock, buffer, MSGSIZE, 0);
            if (count == 0) {
                break;
            }
            puts(buffer);
        } else if (0 == strncmp(buffer, "find", 4) || 0 == strncmp(buffer, "inspect", 7)) {
            count = send(locsock, buffer, strlen(buffer) + 1, 0);
            if (count != strlen(buffer) + 1) {
                logexit("erro no send\n");
            }
            memset(buffer, 0, MSGSIZE);

            count = recv(locsock, buffer, MSGSIZE, 0);
            if (count == 0) {
                break;
            }
            puts(buffer);
        } else {
            printf("unknown command\n");
        }
    }

    close(usersock);
    close(locsock);

    return 0;
}