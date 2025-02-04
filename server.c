#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"

#define MAX_PEERS 1

typedef struct sockaddr_storage sockaddr_storage;
typedef struct sockaddr sockaddr;
typedef struct sockaddr_in6 sockaddr_in6;

typedef struct client_data {
    int sock;
} client_data;

typedef struct p2p_data {
    int port;
} p2p_data;

int peer_id, my_id;
int peersock;
int has_peer;

int active_con;

int n_clients;
Client client_list[MAX_CLIENTS];

int n_users;
User users[MAX_USERS];

int n_locs;
UserLoc userlocs[MAX_USERS];

int find_user_loc_by_id(const char *uid);
int find_user_by_id(const char *uid);

void usage() {
    printf("Exemplo de uso:\n");
    printf("./server <p2p port> <client port>\n");
    exit(EXIT_FAILURE);
}

void config_sock(int sock) {
    // permite o socket receber conexoes ipv4 e ipv6
    int ipv6_only = 0;
    if (0 != setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, &ipv6_only, sizeof(ipv6_only))) {
        logexit("config socket");
    }

    // reuso de socket
    int enable = 1;
    if (0 != setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))) {
        logexit("sockopt");
    }
}

int server_sockaddr_init(uint16_t port, sockaddr_in6 *addr) {
    if (port == 0) {
        return -1;
    }

    memset(addr, 0, sizeof(&addr));
    addr->sin6_family = AF_INET6;
    addr->sin6_addr = in6addr_any;
    addr->sin6_port = port;

    return 0;
}

int start_client_sock(uint16_t port) {
    int sock = socket(AF_INET6, SOCK_STREAM, 0);
    if (sock == -1) {
        logexit("erro no socket");
    }

    config_sock(sock);

    sockaddr_in6 addr;
    if (0 != server_sockaddr_init(port, &addr)) {
        logexit("init client sockaddr");
    }

    sockaddr *saddr = (sockaddr *)(&addr);
    if (0 != bind(sock, saddr, sizeof(addr))) {
        logexit("erro no bind");
    }

    if (0 != listen(sock, MAX_CLIENTS)) {
        logexit("listen");
    }

    return sock;
}

int start_p2p_sock(uint16_t port) {
    int sock = socket(AF_INET6, SOCK_STREAM, 0);
    if (sock == -1) {
        logexit("erro no socket");
    }

    char buffer[MSGSIZE];
    memset(buffer, 0, sizeof(buffer));

    config_sock(sock);

    sockaddr_in6 addr;
    if (0 != server_sockaddr_init(port, &addr)) {
        logexit("init p2p sockaddr");
    }

    if (0 != connect(sock, (sockaddr *)&addr, sizeof(addr))) {
        printf("No peer found, starting to listen...\n");

        if (0 != bind(sock, (sockaddr *)&addr, sizeof(addr))) {
            logexit("Peer bind error");
        }

        if (0 != listen(sock, MAX_PEERS)) {
            logexit("Peer listen error");
        }
    } else {
        recv(sock, buffer, MSGSIZE, 0);
        puts(buffer);

        if (0 == strncmp(buffer, "Peer limit exceeded", 20)) {
            return -1;
        }

        if (0 == strncmp(buffer, "New Peer ID", 11)) {
            sscanf(buffer, "New Peer ID: %d", &my_id);

            peer_id = rand() % 20;
            printf("Peer %d connected\n", peer_id);

            peersock = sock;

            memset(buffer, 0, MSGSIZE);
            sprintf(buffer, "New Peer ID: %d", peer_id);

            send(sock, buffer, MSGSIZE, 0);
        }
    }

    return sock;
}

void kill_client(Client client) {
    printf("Client %d removed (Loc %d)\n", client.id, client.loc);

    int client_pos;
    for (int i = 0; i < n_clients; i++) {
        if (client_list[i].id == client.id) {
            client_pos = i;
            break;
        }
    }

    for (int i = client_pos; i < n_clients - 1; i++) {
        client_list[i] = client_list[i + 1];
    }

    client_list[n_clients - 1] = (Client){-1, -1, 0};

    n_clients--;
}

int handle_peer(int sock) {
    char buffer[MSGSIZE];
    memset(buffer, 0, MSGSIZE);

    size_t count = recv(sock, buffer, MSGSIZE, 0);
    if (count <= 0) {
        return 1;
    }

    if (0 == strncmp(buffer, "New Peer ID", 11)) {
        puts(buffer);
    }

    if (0 == strncmp(buffer, "move", 4)) {
        char uid[11];
        int loc_id = -1;

        if (2 == sscanf(buffer, "move %d %s", &loc_id, uid)) {
            printf("REQ_LOCREG %s %d\n", uid, loc_id);

            int pos = find_user_loc_by_id(uid), last_loc;
            if (pos == -1) {
                UserLoc new_user_loc;
                strcpy(new_user_loc.id, uid);
                new_user_loc.loc_id = loc_id;

                userlocs[n_locs] = new_user_loc;

                last_loc = -1;

                n_locs++;
            } else {
                last_loc = userlocs[pos].loc_id;
                userlocs[pos].loc_id = loc_id;
            }

            sprintf(buffer, "%d", last_loc);

            send(sock, buffer, MSGSIZE, 0);
        }
    }

    if (0 == strncmp(buffer, "auth", 4)) {
        char uid[11];

        if (1 == sscanf(buffer, "auth %s", uid)) {
            printf("REQ_USRAUTH %s\n", uid);

            int pos = find_user_by_id(uid);

            sprintf(buffer, "has auth %d", users[pos].special);
            send(sock, buffer, MSGSIZE, 0);
        }
    }

    return 0;
}

int find_user_by_id(const char *uid) {
    for (int i = 0; i < n_users; i++) {
        if (0 == strcmp(users[i].id, uid)) return i;
    }

    return -1;
}

int find_user_loc_by_id(const char *uid) {
    for (int i = 0; i < n_locs; i++) {
        if (0 == strcmp(userlocs[i].id, uid)) return i;
    }

    return -1;
}

int add_user(const char *uid, int special) {
    int result;

    int pos = find_user_by_id(uid);
    if (pos == -1) {
        if (n_users >= MAX_USERS) return 2;

        User new_user;
        strcpy(new_user.id, uid);
        new_user.special = special;

        users[n_users] = new_user;

        n_users++;

        result = 0;
    } else {
        users[pos].special = special;
        result = 1;
    }

    return result;
}

int find_user(const char *uid) {
    int result;

    int pos = find_user_loc_by_id(uid);
    if (pos == -1) {
        result = 0;
    } else {
        result = userlocs[pos].loc_id;
    }
    return result;
}

int move_user(const char *uid, const char *dir, int loc) {
    int result = -2;

    int pos = find_user_by_id(uid);

    if (pos == -1) {
        result = 0;
    } else {
        char buffer[MSGSIZE];

        int newloc = -1;
        if (0 == strcmp(dir, "in")) {
            newloc = loc;
        }

        sprintf(buffer, "move %d %s", newloc, uid);

        size_t count = send(peersock, buffer, strlen(buffer) + 1, 0);
        if (count != strlen(buffer) + 1) {
            logexit("send");
        }

        count = recv(peersock, buffer, MSGSIZE, 0);

        sscanf(buffer, "%d", &result);
    }

    return result;
}

int get_loc_list(const char *uid, int loc, char *list) {
    int result;

    char buffer[MSGSIZE];
    sprintf(buffer, "auth %s", uid);

    size_t count = send(peersock, buffer, strlen(buffer) + 1, 0);
    if (count != strlen(buffer) + 1) {
        logexit("send");
    }

    count = recv(peersock, buffer, MSGSIZE, 0);

    int auth;
    if (1 == sscanf(buffer, "has auth %d", &auth)) {
        if (auth == 0) return 0;

        for (int i = 0; i < n_locs; i++) {
            if (userlocs[i].loc_id == loc) {
                strcat(list, userlocs[i].id);
                strcat(list, ", ");
            }
        }

        result = 1;
    }

    return result;
}

int handle_client(int sock) {
    char buffer[MSGSIZE];
    memset(buffer, 0, MSGSIZE);

    size_t count = recv(sock, buffer, MSGSIZE, 0);
    if (count <= 0) {
        return 1;
    }

    if (0 == strncmp(buffer, "kill", 4)) {
        Client to_kill = get_client(client_list, n_clients, sock);
        kill_client(to_kill);

        send(sock, buffer, strlen(buffer) + 1, 0);

        return 1;
    }

    if (0 == strncmp(buffer, "add", 3)) {
        char uid[11];
        int special, result;

        if (2 == sscanf(buffer, "add %s %d", uid, &special) && strlen(uid) == 10) {
            printf("REQ_USRADD %s %d\n", uid, special);
            result = add_user(uid, special);
        } else {
            result = -1;
        }

        switch (result) {
            case -1:
                sprintf(buffer, "wrong format");
                break;

            case 0:
                sprintf(buffer, "New user added: %s", uid);
                break;

            case 1:
                sprintf(buffer, "User updated: %s", uid);
                break;

            case 2:
                sprintf(buffer, "User limit exceeded");
                break;
        }
    }

    if (0 == strncmp(buffer, "find", 4)) {
        char uid[11];
        int result;

        if (1 == sscanf(buffer, "find %s", uid) && strlen(uid) == 10) {
            printf("REQ_USRLOC %s\n", uid);
            result = find_user(uid);
        } else {
            result = -2;
        }

        switch (result) {
            case -2:
                sprintf(buffer, "wrong format");
                break;

            case 0:
                sprintf(buffer, "User not found");
                break;

            default:
                sprintf(buffer, "Current location: %d", result);
        }
    }

    if ((0 == strncmp(buffer, "in", 2) && 0 != strncmp(buffer, "inspect", 7)) || 0 == strncmp(buffer, "out", 3)) {
        char uid[11], dir[4];
        int result;

        if (2 == sscanf(buffer, "%s %s", dir, uid)) {
            printf("REQ_USRACCESS %s %s\n", uid, dir);

            Client curr_client = get_client(client_list, n_clients, sock);
            int client_loc = curr_client.loc;

            result = move_user(uid, dir, client_loc);
        } else {
            result = -2;
        }

        switch (result) {
            case -2:
                sprintf(buffer, "wrong format");
                break;

            case 0:
                sprintf(buffer, "User not found");
                break;

            default:
                sprintf(buffer, "Ok. Last location: %d", result);
        }
    }

    if (0 == strncmp(buffer, "inspect", 7)) {
        char uid[11];
        int loc, result;

        char list[331];
        memset(list, 0, 331);

        if (2 == sscanf(buffer, "inspect %s %d", uid, &loc)) {
            printf("REQ_LOCLIST %s %d\n", uid, loc);

            result = get_loc_list(uid, loc, list);
        } else {
            result = -2;
        }

        // remove a virgula e o ultimo espaco
        int len = strlen(list);
        list[len - 2] = '\0';

        switch (result) {
            case -2:
                sprintf(buffer, "wrong format");
                break;

            case 0:
                sprintf(buffer, "Permission denied");
                break;

            default:
                sprintf(buffer, "List of people at the specified location: ");
                strcat(buffer, list);
        }
    }

    count = send(sock, buffer, strlen(buffer) + 1, 0);

    if (count != strlen(buffer) + 1) {
        logexit("send");
    }

    return 0;
}

void kill_server(int has_peer, int peersock) {
    if (!has_peer) return;

    char buffer[MSGSIZE];
    sprintf(buffer, "Peer %d disconnected", my_id);

    // send(peersock, buffer, strlen(buffer) + 1, 0);
}

int main(int argc, char **argv) {
    srand(time(NULL));

    if (argc < 3) {
        usage();
    }

    // get ports
    uint16_t p2p_port, client_port;
    p2p_port = format_port(argv[1]);
    client_port = format_port(argv[2]);

    // start sockets
    int clientsock = start_client_sock(client_port);
    int p2psock = start_p2p_sock(p2p_port);

    if (p2psock == -1) {
        return 1;
    }

    // adiciona os sockets ao set
    fd_set current_socks, ready_socks;
    FD_ZERO(&current_socks);
    FD_SET(clientsock, &current_socks);
    FD_SET(p2psock, &current_socks);
    FD_SET(STDIN_FILENO, &current_socks);

    has_peer = 0;
    n_clients = n_users = n_locs = 0;

    char buffer[MSGSIZE];

    while (1) {
        ready_socks = current_socks;
        memset(buffer, 0, MSGSIZE);

        if (0 > select(FD_SETSIZE, &ready_socks, NULL, NULL, NULL)) {
            logexit("erro no select");
        }

        for (int i = 0; i < FD_SETSIZE; i++) {
            if (FD_ISSET(i, &ready_socks)) {
                if (i == clientsock) {
                    int newclientsock = accept(clientsock, NULL, NULL);

                    if (newclientsock >= 0) {
                        FD_SET(newclientsock, &current_socks);

                        if (0 >= recv(newclientsock, buffer, MSGSIZE, 0)) {
                            logexit("rec loc");
                        }

                        Client new_client;
                        new_client.id = n_clients;
                        new_client.sock = newclientsock;
                        new_client.loc = atoi(buffer);

                        client_list[n_clients] = new_client;
                        n_clients++;

                        printf("Client %d added (Loc %d)\n", new_client.id, new_client.loc);

                        memset(buffer, 0, MSGSIZE);
                        sprintf(buffer, "%d", new_client.id);

                        send(newclientsock, buffer, strlen(buffer) + 1, 0);
                    }
                } else if (i == p2psock) {
                    if (!has_peer) {
                        struct sockaddr_in6 caddr;
                        socklen_t csize = sizeof(caddr);
                        int newpeersock = accept(p2psock, (sockaddr *)&caddr, &csize);

                        if (newpeersock >= 0) {
                            if (has_peer >= MAX_PEERS) {
                                sprintf(buffer, "Peer limit exceeded");
                                send(newpeersock, buffer, strlen(buffer) + 1, 0);
                                close(newpeersock);
                            } else {
                                peer_id = rand() % 20;

                                printf("Peer %d connected\n", peer_id);
                                FD_SET(newpeersock, &current_socks);
                                peersock = newpeersock;

                                sprintf(buffer, "New Peer ID: %d", peer_id);
                                send(newpeersock, buffer, strlen(buffer) + 1, 0);
                                has_peer = 1;
                            }
                        } else {
                            handle_peer(i);
                        }
                    }

                } else if (i == STDIN_FILENO) {
                    char line[MSGSIZE];
                    if (!fgets(line, MSGSIZE - 1, stdin)) {
                        continue;
                    }

                    if (0 == strncmp(line, "kill", 4)) {
                        kill_server(has_peer, peersock);
                        return 1;
                    }
                } else {
                    if (has_peer && i == peersock) {
                        int end_connection = handle_peer(i);

                        if (end_connection) {
                            printf("Peer %d disconnected\n", peer_id);

                            has_peer = 0;
                            my_id = -1;
                            peer_id = -1;

                            printf("No peer found, starting to listen...\n");

                            FD_CLR(i, &current_socks);

                            close(i);
                        }
                    } else {
                        int end_connection = handle_client(i);

                        if (end_connection) {
                            FD_CLR(i, &current_socks);

                            close(i);
                        }
                    }
                }
            }
        }
    }
    close(clientsock);
    close(p2psock);

    exit(EXIT_SUCCESS);
    return 0;
}