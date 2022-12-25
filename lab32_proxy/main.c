#include <stdio.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/poll.h>

#include "TaskList.h"
#include "threadArguments.h"
#include "read_request.h"
#include "write_request.h"
#include "read_response.h"
#include "write_response.h"
#include "Cache.h"

#define MAX_CLIENTS_QUANTITY 8192;

char proxy_shutdown = 0;
char compress_array = 0;

size_t connections_array_size = 1020;
struct taskList tasks;
struct workThreadArguments threadArguments;
pthread_attr_t pthreadAttr;

int accept_clients(nfds_t* client_qty, const int listen_sfd, struct pollfd* connections){
    printf("accepting\n");
    char err_msg[255];
    if(*client_qty < connections_array_size){
        int new_sfd = accept(listen_sfd, NULL, NULL);
        if(new_sfd == -1){
            strerror_r(errno, err_msg, 255);
            printf("accept failed: %s\n", err_msg);
            return -1;
        }

        connections[*client_qty].fd = new_sfd;
        connections[*client_qty].events = POLLIN | POLLOUT;
        ++*client_qty;
    }
    else{
        //realloc or refuse clients connections
    }
    return 0;
}
int handleEvents(struct pollfd* connections, const nfds_t* connections_qty){
    int handled = 0;
    for(int i = 0; i < *connections_qty; ++i){
        if(connections[i].revents == 0){
            continue;
        }
        struct task* task1 = tasks.pending_tasks[connections[i].fd];
        tasks.pending_tasks[connections[i].fd] = NULL;
        pthread_mutex_lock(&threadArguments.rwlock);
        pthread_t new_thread;
        if(connections[i].revents & (POLLIN | POLLOUT)){
            //printf("newTask added\ntype: %d\n", type);
            if(task1){
                switch (task1->type) {
                    case WRITE_REQUEST:
                        pthread_create(&new_thread, &pthreadAttr, write_request, task1);
                        break;
                    case READ_RESPONSE:
                        pthread_create(&new_thread, &pthreadAttr, read_response, task1);
                        break;
                    case WRITE_RESPONSE:
                        pthread_create(&new_thread, &pthreadAttr, write_response, task1);
                        break;
                    default:
                        break;

                }
            }
            else{
                pthread_create(&new_thread, &pthreadAttr, read_request, createTask(REQUEST, connections[i].fd, &threadArguments));
            }
            connections[i].fd *= -1;
            compress_array = 1;
            --handled;
        }
        else if(connections[i].revents & (POLLHUP | POLLERR)){
            printf("POLLHUP or POLERR\n");
            if(task1){
                deleteTaskFromPendingTasks(&tasks, connections[i].fd);
            }
            else{
                close(connections[i].fd);
                connections[i].fd *= -1;
                compress_array = 1;
                --handled;
            }
        }
        pthread_mutex_unlock(&threadArguments.rwlock);
    }
    return handled;
}
void compress(struct pollfd* arr, nfds_t size){
    int offset = 0;
    for(int i = 0; i < size; ++i){
        while (arr[i + offset].fd < 0){
            ++offset;
        }
        if(offset){
            arr[i].fd = arr[i + offset].fd;
            arr[i].events = arr[i + offset].events;
        }
    }
    compress_array = 0;
}
int main(int argc, char* argv[]) {
    if(argc != 2){
        printf("wrong arguments quantity");
        exit(EXIT_FAILURE);
    }
    char err_msg[255];
    int listen_sfd, listen_port;

    if(pthread_mutex_init(&threadArguments.rwlock, NULL) != 0){
        printf("rwlock init failed\n");
        exit(EXIT_FAILURE);
    }

    listen_port = atoi(argv[1]);
    if(listen_port < 1024){
        printf("bad input");
        exit(EXIT_FAILURE);
    }
    if((listen_sfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        strerror_r(errno, err_msg, 255);
        printf("creating listen socket failed: %s\n", err_msg);
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in proxy_address;
    proxy_address.sin_addr.s_addr = INADDR_ANY;
    proxy_address.sin_family = AF_INET;
    proxy_address.sin_port = htons(listen_port);

    if(bind(listen_sfd, (struct sockaddr*)&proxy_address, sizeof(proxy_address)) == -1){
        strerror_r(errno, err_msg, 255);
        printf("bind failed: %s\n", err_msg);
        close(listen_sfd);
        exit(EXIT_FAILURE);
    }
    if(listen(listen_sfd, 10) == -1){
        strerror_r(errno, err_msg, 255);
        printf("listening failed: %s\n", err_msg);
        close(listen_sfd);
        exit(EXIT_FAILURE);
    }


    nfds_t nfds = 0;
    struct pollfd* connections = malloc(sizeof(struct pollfd) * (connections_array_size + 1));
    if(connections == NULL){
        printf("malloc failed.\n");
        exit(EXIT_FAILURE);
    }

    connections[0].fd = listen_sfd;
    connections[0].events = POLLIN;


    TaskList_init(&tasks, connections_array_size);
    threadArguments.tl = &tasks;
    threadArguments.connections = &connections[1];
    threadArguments.connections_qty = &nfds;
    threadArguments.max_connections_qty = &connections_array_size;
    threadArguments.cache = cacheInit(connections_array_size);

    pthread_attr_setdetachstate(&pthreadAttr, PTHREAD_CREATE_DETACHED);

    int timeout = 500;
    int err;

    nfds_t current_nfds = nfds;
    int handled_qty = 0;
    for(;;){

        pthread_mutex_lock(&threadArguments.rwlock);
        current_nfds = nfds + handled_qty;
        if(compress_array){
            compress(connections, current_nfds);
        }
        nfds = current_nfds;
        pthread_mutex_unlock(&threadArguments.rwlock);

        handled_qty = 0;

        //printf("pollin %ld connections\n", nfds);
        err = poll(connections, current_nfds + 1, timeout);
        if(err == -1){
            strerror_r(errno, err_msg, 255);
            printf("poll failed: %s\n", err_msg);
            proxy_shutdown = 1;
            break;
        }
        else if(err == 0){
            if(compress_array){
                pthread_mutex_lock(&threadArguments.rwlock);
                compress(connections, nfds);
                pthread_mutex_unlock(&threadArguments.rwlock);
            }
            continue;
        }
        else{
            handled_qty = handleEvents(&connections[1], &current_nfds);
            if(connections[0].revents == POLLIN){
                pthread_mutex_lock(&threadArguments.rwlock);
                accept_clients(&nfds, listen_sfd ,&connections[1]);
                pthread_mutex_unlock(&threadArguments.rwlock);
            }
        }



    }
    if(proxy_shutdown){
        for(int i = 1; i < nfds; ++i){
            close(connections[i].fd);
        }
        close(listen_sfd);
        return EXIT_FAILURE;
    }

    return 0;
}
