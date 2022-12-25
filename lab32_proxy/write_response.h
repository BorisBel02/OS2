//
// Created by boris on 25/12/22.
//

#ifndef LAB32_PROXY_WRITE_RESPONSE_H
#define LAB32_PROXY_WRITE_RESPONSE_H

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <malloc.h>

#include "threadArguments.h"

void* write_response(void* args);

#endif //LAB32_PROXY_WRITE_RESPONSE_H
