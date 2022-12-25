#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

pthread_mutex_t mutexes[3];

void* child_thread(){

    pthread_mutex_lock(&mutexes[0]);
    pthread_mutex_unlock(&mutexes[0]);

    int err;
    for(int i = 0; i < 10; ++i){
        if((err = pthread_mutex_lock(&mutexes[2])) != 0){
            printf("%s", strerror(err));
            exit(EXIT_FAILURE);
        }
        printf("child thread text!\n");
        if((err = pthread_mutex_unlock(&mutexes[1])) != 0){
            printf("%s", strerror(err));
            exit(EXIT_FAILURE);
        }
    }

    pthread_exit(NULL);
}
int main() {
    int err;
    for(int i = 0; i < 3; ++i){
        pthread_mutex_init(&mutexes[i], NULL);
    }

    pthread_t thread;

    pthread_mutex_lock(&mutexes[0]);
    if(pthread_create(&thread, NULL, child_thread, NULL) != 0){
        perror("pthread_create failed");
        return errno;
    }

    if((err = pthread_mutex_lock(&mutexes[2])) != 0){
        printf("%s", strerror(err));
        exit(EXIT_FAILURE);
    }
    if((err = pthread_mutex_unlock(&mutexes[0])) != 0){
        printf("%s", strerror(err));
        exit(EXIT_FAILURE);
    }
    for(int i = 0; i < 10; ++i){
        if((err = pthread_mutex_lock(&mutexes[1])) != 0){
            printf("%s", strerror(err));
            exit(EXIT_FAILURE);
        }
        printf("master thread text!\n");
        if((err = pthread_mutex_unlock(&mutexes[2])) != 0){
            printf("%s", strerror(err));
            exit(EXIT_FAILURE);
        }
    }

    void* res = NULL;
    if(pthread_join(thread, &res) != 0){
        perror("join failed");
    }
    printf("%s", (char*)res);

    pthread_exit(NULL);
}
