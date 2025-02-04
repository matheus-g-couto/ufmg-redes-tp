#pragma once

#include <arpa/inet.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>

typedef struct sockaddr_storage sockaddr_storage;
typedef struct sockaddr sockaddr;

#ifndef COMMON_H
#define COMMON_H

#define REQ_CONNPEER 17
#define RES_CONNPEER 18
#define REQ_DISCPEER 19
#define REQ_CONN 20
#define RES_CONN 21
#define REQ_DISC 22

#define REQ_USRADD 33
#define REQ_USRACCESS 34
#define RES_USRACCESS 35
#define REQ_LOCREG 36
#define RES_LOCREG 37
#define REQ_USRLOC 38
#define RES_USRLOC 39
#define REQ_LOCLIST 40
#define RES_LOCLIST 41
#define REQ_USRAUTH 42
#define RES_USRAUTH 43

#define ERROR 255
#define OK 0

#define MSGSIZE 500
#define MAX_USERS 30
#define MAX_CLIENTS 10

#endif

typedef struct {
    int id;
    int sock;
    int loc;
} Client;

typedef struct {
    char id[11];
    int special;
} User;

typedef struct {
    char id[11];
    int loc_id;
} UserLoc;

void logexit(const char* msg);

uint16_t format_port(const char* portstr);

int addrparse(const char* addrstr, uint16_t port, sockaddr_storage* storage);

Client get_client(Client* clist, int n, int sock);