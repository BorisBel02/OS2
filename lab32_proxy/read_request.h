//
// Created by boris on 25/12/22.
//

#ifndef LAB32_PROXY_READ_REQUEST_H
#define LAB32_PROXY_READ_REQUEST_H

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <arpa/inet.h>

#include "threadArguments.h"

void* read_request(void* args);

#endif //LAB32_PROXY_READ_REQUEST_H
