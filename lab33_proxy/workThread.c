//
// Created by boris on 11/12/22.
//

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "workThread.h"


struct httpRequestProperties{
    char* url;
    char* host;
    char* port;
    char* method;
    char* version;
}httpRequestProperties;
void freeHttpRequestProperties(struct httpRequestProperties* properties){

    if(properties->url){
        free(properties->url);
    }
    if(properties->host){
        free(properties->host);
    }
    if(properties->port){
        free(properties->port);
    }
    if(properties->version){
        free(properties->version);
    }
    free(properties);
}
struct httpRequestProperties* parseHttp(char* request, size_t request_size){
    //printf("REQUEST:\n%s", request);
    char* buf;
    char* end_of_headers = strstr(request, "\r\n\r\n");
    if(end_of_headers == NULL){
        return NULL;
    }
    buf = (char*)malloc(sizeof(char) * (end_of_headers - request + 1));
    memcpy(buf, request, (end_of_headers - request) * sizeof(char));
    buf[end_of_headers - request + 1] = '\0';


    //Getting one line with \r\n delimiter
    char* line, * saveptr1, * strbuf;
    strbuf = buf;
    line = strtok_r(strbuf, "\r\n", &strbuf);
    if(line == NULL){
        printf("bad request\n");
        free(buf);
        return NULL;
    }
    //parsing first line
    char* token, *saveptr2, *linebuf;
    linebuf = line;
    token = strtok_r(linebuf, " ", &saveptr2);
    if(token == NULL){
        printf("bad request\n");
        free(buf);
        return NULL;
    }
    if((strcmp(token, "GET")) != 0){
        printf("Only GET allowed\n");
        free(buf);
        return NULL;
    }
    linebuf = NULL;
    token = strtok_r(linebuf, " ", &saveptr2);
    if(token == NULL){
        printf("No URL found.\n");
        free(buf);
        return NULL;
    }
    struct httpRequestProperties* props = (struct httpRequestProperties*)malloc(sizeof(struct httpRequestProperties));
    props->url = NULL;
    props->host = NULL;
    props->port = NULL;
    props->method = NULL;
    props->version = NULL;

    props->url = (char*) malloc(sizeof(char) * (strlen(token) + 1));
    memcpy(props->url, token, (strlen(token)) * sizeof(char));
    props->url[strlen(token) + 1] = '\0';

    linebuf = NULL;
    token = strtok_r(linebuf, " ", &saveptr2);
    if(token == NULL){
        free(props->url);
        free(props);
        free(buf);
        printf("Version not specified.\n");
        return NULL;
    }
    /*if(token[5] != '1'){
        free(props->url);
        free(props);
        free(buf);
        printf("version: %s is not supported\n", token);
        return NULL;
    }*/
    if(strbuf == NULL){
        printf("parsing failed\n");
        free(props->url);
        free(props);
        free(buf);
        return NULL;
    }
    props->version = malloc(sizeof(char) * (strlen(token - 4)));
    strncpy(props->version, &token[5], strlen(token) - 5);


    char host_found = 0;

    for(;host_found == 0;strbuf = NULL){
        line = strtok_r(strbuf, "\r\n", &saveptr1);
        if(line == NULL){
            break;
        }
        linebuf = line;
        token = strtok_r(linebuf, ": ", &saveptr2);
        if(strcmp(token, "Host") == 0){
            host_found = 1;
            linebuf = NULL;
            break;
        }
        /*for(linebuf = line; host_found == 0; linebuf = NULL){
            token = strtok_r(linebuf, " ", &saveptr2);
            if(strcmp(token, "Host:") == 0){
                host_found = 1;
                linebuf = NULL;
                break;
            }
            break;
        }*/
    }
    if(host_found == 0){
        free(props->url);
        free(props);
        free(buf);
        printf("Host is not specified.\n");
        return NULL;
    }
    token = strtok_r(linebuf, ": ", &saveptr2);
    if(token == NULL){
        free(props->url);
        free(props);
        free(buf);
        printf("Host not specified.\n");
        return NULL;
    }
    props->host = (char*)malloc(sizeof(char) * (strlen(token) + 1));
    memcpy(props->host, token, sizeof(char) * (strlen(token)));
    props->host[strlen(token) + 1] = '\0';

    linebuf = NULL;
    token = strtok_r(linebuf, ": ", &saveptr2);
    if(token){
        props->port = (char*)malloc(sizeof(char) * (strlen(token) + 1));
        memcpy(props->port, token, sizeof(char) * strlen(token));
        props->port[strlen(token) + 1] = '\0';
    }
    else{
        props->port = (char*)malloc(sizeof(char) * (3));
        strcpy(props->port, "80");
        props->port[2] = '\0';
    }
    free(buf);
    return props;
}
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
void* workThread(void* args){
    char err_msg[255];
    int err_code;
    char msg[BUFSIZ] = { 0 };
    struct workThreadArguments* arguments = (struct workThreadArguments*)args;
    struct taskList* tasklist = arguments->tl;

    struct task* current_task;
    for(;;){
        current_task = getTask(tasklist);
        if(current_task == NULL){
            while(tasklist->node_qty == 0){
                pthread_mutex_lock(&tasklist->cond_mutex);
                //printf("I'll wait!\n");
                pthread_cond_wait(&tasklist->task_present_notification, &tasklist->cond_mutex);
                //printf("WAKE UP!\n");
                pthread_mutex_unlock(&tasklist->cond_mutex);
            }
            continue;
        }
        if(current_task->type == REQUEST){
            printf("handling REQUEST\n");
            ssize_t msg_size = 0;
            if((msg_size = read(current_task->client_sfd, msg, BUFSIZ)) == -1){
                strerror_r(errno, err_msg, 255);
                printf("read failes %s on desc: %d\n", err_msg, current_task->client_sfd);
                close(current_task->client_sfd);
                free(current_task);
                continue;
            }
            if(msg_size == 0){
                close(current_task->client_sfd);
                free(current_task);
                continue;
            }
            printf("parsing\n");
            struct httpRequestProperties* properties = parseHttp(msg, msg_size);
            if(properties == NULL){
                close(current_task->client_sfd);
                free(current_task);
                continue;
            }
            //cache checking
            struct HashTableCell* response;
            printf("cachekey\n");
            char* cachekey = (char*)malloc(sizeof(char) * (strlen(properties->url) + strlen(properties->host) + 1));
            if(cachekey){
                strcpy(cachekey, properties->host);
                strcpy(&cachekey[strlen(properties->host)], properties->url);
                printf("key created\n");
                cachekey[strlen(properties->url) + strlen(properties->host) + 1] = '\0';
                //printf("host: %s\nurl:%s\n", properties->host, properties->url);
                //printf("key: %s\n", cachekey);
                response = getCachedResponse(cachekey, arguments->cache);
                printf("got cache\n");
                if(response){   //maybe create WRITE_RESPONSE task
                    printf("CACHE HIT! Writing response from cache\n");
                    if(write(current_task->client_sfd, response->response_string, response->response_size) < 0){
                        printf("write to client failed\n");
                    }
                    printf("free stuff\n");
                    freeHttpRequestProperties(properties);
                    free(cachekey);
                    free(current_task);
                    continue;
                }
            }
            else{
                printf("malloc failed\n");
                freeHttpRequestProperties(properties);
                close(current_task->client_sfd);
                free(current_task);
                continue;
            }
            int server_sfd;
            struct addrinfo hints;
            struct addrinfo* result, * rp;
            memset(&hints, 0, sizeof(hints));
            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_STREAM;
            hints.ai_flags = 0;
            hints.ai_protocol = 0;

            printf("getting addr info for host %s\n", properties->host);
            err_code = getaddrinfo(properties->host, properties->port, &hints, &result);
            if(err_code != 0){
                printf("getaddrinfo failed:\n%s\n", gai_strerror(err_code));
                close(current_task->client_sfd);
                freeHttpRequestProperties(properties);
                free(current_task);
                continue;
            }
            printf("CONNECTING...\n");
            char ip[50];
            for (rp = result; rp != NULL; rp = rp->ai_next) {
                server_sfd = socket(rp->ai_family, rp->ai_socktype,
                             rp->ai_protocol);
                if (server_sfd == -1)
                    continue;
                inet_ntop(AF_INET, &rp->ai_addr->sa_data[2], ip, 50);
                printf("connecting to ip: %s\n", ip);
                if (connect(server_sfd, rp->ai_addr, rp->ai_addrlen) != -1)
                    break;                  /* Success */

                close(server_sfd);
            }

            if(rp == NULL){
                strerror_r(errno, err_msg, 255);
                printf("connection to host %s failed:\n%s\n", properties->host ,err_msg);
                freeHttpRequestProperties(properties);
                close(current_task->client_sfd);
                free(current_task);
                freeaddrinfo(result);
                continue;
            }
            freeaddrinfo(result);
            //printf("CONNECTED!\n");
            /*if(write(server_sfd, msg, msg_size) == -1){
                strerror_r(errno, err_msg, 255);
                printf("error on writing to server: %s\n", err_msg);
                close(server_sfd);
                close(current_task->client_sfd);
                freeHttpRequestProperties(properties);
                free(current_task);
                continue;
            }*/ //now there is new task type for this
            if(server_sfd < *arguments->max_connections_qty){
                current_task->server_sfd = server_sfd;
                current_task->type = WRITE_REQUEST;
                current_task->string = malloc(msg_size * sizeof(char));
                memcpy(current_task->string, msg, msg_size * sizeof(char));
                current_task->string_size = msg_size;
                current_task->cache_key = cachekey;
                pendingTaskAppend(arguments->tl, current_task, server_sfd);

                pthread_mutex_lock(&arguments->rwlock);
                arguments->connections[*arguments->connections_qty].fd = server_sfd;
                arguments->connections[*arguments->connections_qty].events = POLLOUT;
                ++*arguments->connections_qty;
                pthread_mutex_unlock(&arguments->rwlock);

                printf("new server conn: %d\n", server_sfd);
            }
            else{
                printf("socket num greater than array size\n");
                close(current_task->client_sfd);
                close(server_sfd);
                free(current_task);
            }

            freeHttpRequestProperties(properties);
        }
        else if(current_task->type == WRITE_REQUEST){
            if(write(current_task->server_sfd, current_task->string, current_task->string_size) == -1){
                strerror_r(errno, err_msg, 255);
                printf("writing request failed.\nERROR: %s", err_msg);
                close(current_task->server_sfd);
                close(current_task->client_sfd);
                free(current_task->string);
                free(current_task->cache_key);
                free(current_task);
                continue;
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
        else if(current_task->type == READ_RESPONSE){
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
                    printf("memcpy\n");
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
                free(current_task);
                continue;
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
        else{
            printf("Handling WRITE_RESPONSE\n");
            if(write(current_task->client_sfd, current_task->string, current_task->string_size) < -1){
                strerror_r(errno, err_msg, 255);
                printf("writing response failed.\nERROR: %s\n", err_msg);
            }
            close(current_task->client_sfd);
            free(current_task->string);
            printf("freeing current task\n");
            free(current_task);
        }
        printf("task handled!\n");
    }
}

