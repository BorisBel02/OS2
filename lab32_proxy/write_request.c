//
// Created by boris on 25/12/22.
//

#include "write_request.h"

void* write_request(void* args){
    char err_msg[255];

    struct task* current_task = (struct task*)args;

    struct workThreadArguments* arguments = current_task->arguments;

    if(write(current_task->server_sfd, current_task->string, current_task->string_size) == -1){
        strerror_r(errno, err_msg, 255);
        printf("writing request failed.\nERROR: %s", err_msg);
        close(current_task->server_sfd);
        close(current_task->client_sfd);
        free(current_task->string);
        free(current_task->cache_key);
        free(current_task);
        return NULL;
    }

    current_task->type = READ_RESPONSE;
    free(current_task->string);
    current_task->string = NULL;
    current_task->string_size = 0;
    pendingTaskAppend(arguments->tl, current_task, current_task->server_sfd);

    pthread_mutex_lock(&arguments->rwlock);
    arguments->connections[*arguments->connections_qty].fd = current_task->server_sfd;
    arguments->connections[*arguments->connections_qty].events = POLLIN;
    ++*arguments->connections_qty;
    pthread_mutex_unlock(&arguments->rwlock);

}