//
// Created by boris on 19/09/22.
//

#include "regFileHandler.h"

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "arg_path.h"

void* regFileHandler(void* arg){
    arg_path* argPath = (arg_path*)arg;
    char err_msg[256];
    //printf("copying file %s\n", argPath->src_path);

    int dst_fd, src_fd;
    int failure_counter = 0;
    while(((src_fd = open(argPath->src_path, O_RDONLY)) == -1) && failure_counter <= 3){
        if((errno == EMFILE || errno == ENFILE) && failure_counter <= 3){
            ++failure_counter;
            sleep(5);
        }
        else{
            strerror_r(errno, err_msg, 255);
            printf("can not open file %s\nerror: %s\n", argPath->src_path, err_msg);
            free(argPath->src_path);
            free(argPath->dst_path);
            free(argPath);
            pthread_exit(NULL);
        }
    }
    failure_counter = 0;
    //printf("open file %s\n", argPath->dst_path);
    if(((dst_fd = open(argPath->dst_path, O_CREAT | O_WRONLY)) == -1) && failure_counter <= 3){
        if((errno == EMFILE || errno == ENFILE)){
            ++failure_counter;
            sleep(5);
        }
        else{
            strerror_r(errno, err_msg, 255);
            printf("can not create file %s\nerror: %s\n", argPath->dst_path, err_msg);
            free(argPath->src_path);
            free(argPath->dst_path);
            free(argPath);
            close(src_fd);
            pthread_exit(NULL);
        }
    }

    char* buffer[BUFSIZ] = { 0 };
    ssize_t sym_qty;
    while((sym_qty = read(src_fd, buffer, BUFSIZ)) > 0){
        //printf("read %ld symbols\n", sym_qty);
        //printf("%s\n", buffer);
        if(write(dst_fd, buffer, sym_qty) == -1){
            strerror_r(errno, err_msg, 255);
            printf("write in %s failed\nerror: %s\n", argPath->dst_path, err_msg);
            free(argPath->src_path);
            free(argPath->dst_path);
            free(argPath);
            close(dst_fd);
            close(src_fd);
            pthread_exit(NULL);
        }
    }

    struct stat file_stats;
    if(fstat(src_fd, &file_stats) == -1){
        strerror_r(errno, err_msg, 255);
        printf("stat failed\nerror: %s", err_msg);
        free(argPath->src_path);
        free(argPath->dst_path);
        free(argPath);
        pthread_exit(NULL);
    }

    if(fchmod(dst_fd, file_stats.st_mode) == -1){
        strerror_r(errno, err_msg, 255);
        printf("chmod failed\nerror: %s\n", err_msg);
    }
    free(argPath->src_path);
    free(argPath->dst_path);
    free(argPath);
    close(dst_fd);
    close(src_fd);
    pthread_exit(NULL);

}