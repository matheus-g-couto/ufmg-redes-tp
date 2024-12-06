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

#endif

void logexit(const char* msg);

int addrparse(const char* addrstr, const char* portstr, sockaddr_storage* storage);

void addrtostr(const sockaddr* addr, char* str, size_t strsize);

int server_sockaddr_init(const char* protocol, const char* portstr, sockaddr_storage* storage);
