//
// Created by boris on 25/12/22.
//

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "read_response.h"

char isResponseOK(char* response){
    char* end_of_line = strstr(response, "\r\n");
    if(end_of_line == NULL){
        return 0;
    }
    char* buf = (char*) malloc(sizeof(char) * (end_of_line - response + 1));
    strncpy(buf, response, end_of_line - response);
    buf[end_of_line - response] = '\0';

    char* token;
    char* saveptr;

    token = strtok_r(buf, " ", &saveptr);
    token = strtok_r(saveptr, " ", &saveptr);
    if(strcmp(token, "200") != 0){
        free(buf);
        return 0;
    }
    free(buf);
    return 1;
}

void* read_response(void* args){
    char err_msg[255];
    int err_code;
    char msg[BUFSIZ] = { 0 };

    struct task* current_task = (struct task*)args;

    struct workThreadArguments* arguments = current_task->arguments;
    printf("Handling READ_RESPONSE\n");

    ssize_t msg_size;
    char* response = NULL;
    size_t response_size = 1;
    char cache_ok = 1;
    while((msg_size = read(current_task->server_sfd, msg, BUFSIZ)) > 0){
        printf("read\n");
        if(response_size == 1){
            cache_ok = isResponseOK(msg);
        }
        printf("RESPONSE\n");
        printf("allocating %ld bytes for response\n", (response_size + msg_size - 1));
        response = (char*) realloc(response, sizeof(char) * (response_size + msg_size - 1));
        if(response){
            memcpy(&response[response_size - 1], msg, msg_size * sizeof(char));
            response_size += msg_size;
        }
        else{
            strerror_r(errno, err_msg, 255);
            printf("adding to cache failed due to malloc failure: %s\n", err_msg);
            cache_ok = 0;
            break;
        }
        printf("read part of response\n");
    }
    printf("out of reading\n");
    if(msg_size == -1){
        strerror_r(errno, err_msg, 255);
        printf("error on reading response: %s\n", err_msg);
        free(response);
        response = NULL;
        close(current_task->server_sfd);
        close(current_task->client_sfd);
        cache_ok = 0;
    }
    /*
     ADD RESPONSE TO CACHE
     */
    if(cache_ok){
        printf("adding response to cache\n");
        pthread_mutex_lock(&arguments->rwlock);
        err_code = addToCache(current_task->cache_key, response, response_size, arguments->cache);
        pthread_mutex_unlock(&arguments->rwlock);
    }
    if(response == NULL || err_code != 0){
        printf("response is NULL\n");
        close(current_task->server_sfd);
        close(current_task->client_sfd);
        free(current_task->cache_key);
        current_task->cache_key = NULL;
        free(current_task);
        current_task = NULL;
        return NULL;
    }

    printf("adding new task\n");
    current_task->type = WRITE_RESPONSE;
    current_task->string = response;
    current_task->string_size = response_size;
    close(current_task->server_sfd);
    current_task->server_sfd *= -1;
    free(current_task->cache_key);
    current_task->cache_key = NULL;
    pendingTaskAppend(arguments->tl, current_task, current_task->client_sfd);

    printf("task appended\n");
    pthread_mutex_lock(&arguments->rwlock);
    arguments->connections[*arguments->connections_qty].fd = current_task->client_sfd;
    arguments->connections[*arguments->connections_qty].events = POLLOUT;
    ++*arguments->connections_qty;
    pthread_mutex_unlock(&arguments->rwlock);
}