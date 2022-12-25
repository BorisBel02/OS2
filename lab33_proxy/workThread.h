//
// Created by boris on 11/12/22.
//

#ifndef LAB33_PROXY_WORKTHREAD_H
#define LAB33_PROXY_WORKTHREAD_H


#include <sys/poll.h>

#include "TaskList.h"
#include "Cache.h"

struct workThreadArguments{
    pthread_mutex_t rwlock;
    struct taskList* tl;
    struct pollfd* connections;
    nfds_t* connections_qty;
    size_t* max_connections_qty;

    struct HashTable* cache;
};
void* workThread(void* args);

#endif //LAB33_PROXY_WORKTHREAD_H
