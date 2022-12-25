#include <stdio.h>
#include <pthread.h>
#include <errno.h>

pthread_cond_t condition;
pthread_mutex_t cond_mutex;

char flag = 1;
void* child_thread(){
    for(int i = 0; i < 10; ++i){
        pthread_mutex_lock(&cond_mutex);
        while(flag == 1){
            pthread_cond_wait(&condition, &cond_mutex);
        }
        printf("%d child thread text!\n", i);
        flag = 1;
        pthread_mutex_unlock(&cond_mutex);
        pthread_cond_signal(&condition);
    }
    pthread_exit(NULL);
}
int main() {
    pthread_t thread;

    pthread_cond_init(&condition, NULL);
    pthread_mutex_init(&cond_mutex, NULL);


    if(pthread_create(&thread, NULL, child_thread, NULL) != 0){
        perror("pthread_create failed");
        return errno;
    }

    for(int i = 0; i < 10; ++i){
        pthread_mutex_lock(&cond_mutex);
        while(flag == 0){
            pthread_cond_wait(&condition, &cond_mutex);
        }
        printf("%d master thread text!\n", i);
        flag = 0;
        pthread_mutex_unlock(&cond_mutex);
        pthread_cond_signal(&condition);
    }

    pthread_cond_destroy(&condition);
    pthread_mutex_destroy(&cond_mutex);

    void* res = NULL;
    if(pthread_join(thread, &res) != 0){
        perror("join failed");
    }
    printf("%s", (char*)res);

    pthread_exit(NULL);
}
