//
// Created by boris on 25/12/22.
//

#ifndef LAB32_PROXY_THREADARGUMENTS_H
#define LAB32_PROXY_THREADARGUMENTS_H

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
#endif //LAB32_PROXY_THREADARGUMENTS_H
