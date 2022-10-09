//
// Created by boris on 19/09/22.
//

#include "dirHandler.h"


#include <dirent.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <thread_db.h>
#include <malloc.h>
#include <sys/stat.h>

#include "arg_path.h"
#include "regFileHandler.h"

void* dir_handler(void* arg){
    arg_path* argPath = (arg_path*)arg;
    char err_msg[256];
    DIR* dir;
    int failure_counter = 0;

    while(((dir = opendir(argPath->src_path)) == NULL) && failure_counter <=3){
        if(errno == ENFILE || errno == EMFILE){
            ++failure_counter;
            sleep(5);
        }
        else{
            strerror_r(errno, err_msg, 255);
            printf("failed to open directory %s\nerror: %s\n", argPath->src_path, err_msg);
            free(argPath->src_path);
            free(argPath->dst_path);
            free(argPath);
            pthread_exit(NULL);
        }
    }

    struct stat dir_stat;
    if(stat(argPath->src_path, &dir_stat) == -1){
        strerror_r(errno, err_msg, 255);
        printf("stat failed\nerror: %s\n", err_msg);
        free(argPath->src_path);
        free(argPath->dst_path);
        free(argPath);
        pthread_exit(NULL);
    }
    if(mkdir(argPath->dst_path, dir_stat.st_mode) != 0){
        strerror_r(errno, err_msg, 255);
        printf("failed to create directory %s\nerror: %s\n", argPath->dst_path, err_msg);
        free(argPath->src_path);
        free(argPath->dst_path);
        free(argPath);
        pthread_exit(NULL);
    }

    struct dirent* de;
    thread_t new_thread;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    //I'm confused in selecting between readdir_r and readdir: man page on the readdir_r forcing to use readdir even in multithread programs(readdir is not thread safe if several threads trying to read from one directory)
    struct dirent* buf = malloc(sizeof(struct dirent) + (fpathconf(dirfd(dir), _PC_NAME_MAX)));
    if(!buf){
        strerror_r(errno, err_msg, 255);
        printf("malloc failed\nerror: %s\n", err_msg);
        closedir(dir);
        free(argPath->src_path);
        free(argPath->dst_path);
        free(argPath);
        pthread_exit(NULL);
    }
    int err = 0;
    while ((err = readdir_r(dir, buf, &de)) == 0){
        if(de == NULL){
            break;
        }
        if(de->d_type == DT_REG){
            arg_path* new_args = (arg_path*)malloc(sizeof (arg_path));

            size_t name_size = strlen(de->d_name);
            new_args->src_path_size = argPath->src_path_size + name_size;
            new_args->dst_path_size = argPath->src_path_size + name_size;

            new_args->src_path = (char*) malloc(new_args->src_path_size + 1);
            strcpy(new_args->src_path, argPath->src_path);
            strcpy(&new_args->src_path[argPath->src_path_size], de->d_name);
            new_args->src_path[argPath->src_path_size - 1] = '/';

            new_args->dst_path = (char*) malloc(new_args->dst_path_size + 1);
            strcpy(new_args->dst_path, argPath->dst_path);
            strcpy(&new_args->dst_path[argPath->dst_path_size], de->d_name);
            new_args->dst_path[argPath->dst_path_size - 1] = '/';

            pthread_create(&new_thread, &attr, regFileHandler, new_args);
        }
        else if(de->d_type == DT_DIR && (strcmp(de->d_name, "..") != 0) && (strcmp(de->d_name, ".") != 0)){
            arg_path* new_args = (arg_path*)malloc(sizeof (arg_path));

            size_t name_size = strlen(de->d_name) + 1;
            new_args->src_path_size = argPath->src_path_size + name_size;
            new_args->dst_path_size = argPath->src_path_size + name_size;

            new_args->src_path = (char*) malloc(new_args->src_path_size);
            strcpy(new_args->src_path, argPath->src_path);
            new_args->src_path[argPath->src_path_size - 1] = '/';
            strcpy(&new_args->src_path[argPath->src_path_size], de->d_name);

            new_args->dst_path = (char*) malloc(new_args->dst_path_size);
            strcpy(new_args->dst_path, argPath->dst_path);
            strcpy(&new_args->dst_path[argPath->dst_path_size], de->d_name);
            new_args->dst_path[argPath->dst_path_size - 1] = '/';

            pthread_create(&new_thread, &attr, dir_handler, new_args);
        }
    }
    if(de != NULL){
        strerror_r(err, err_msg, 255);
        printf("malloc failed\nerror: %s\n", err_msg);
        closedir(dir);
        free(argPath->src_path);
        free(argPath->dst_path);
        free(argPath);
        free(buf);
        pthread_exit(NULL);
    }
    closedir(dir);
    pthread_attr_destroy(&attr);

    free(argPath->src_path);
    free(argPath->dst_path);
    free(argPath);
    free(buf);

    pthread_exit(NULL);
}