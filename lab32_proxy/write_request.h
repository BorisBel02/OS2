//
// Created by boris on 25/12/22.
//

#ifndef LAB32_PROXY_WRITE_REQUEST_H
#define LAB32_PROXY_WRITE_REQUEST_H

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "threadArguments.h"

void* write_request(void* args);

#endif //LAB32_PROXY_WRITE_REQUEST_H
