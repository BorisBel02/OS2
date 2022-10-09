#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>

void* child_routine(void* args){
    while(1){
        printf("I AM STILL ALIVE!!!\n");
        printf("FINISH\n");
        pthread_testcancel();
    }
    pthread_exit(NULL);
}
int main() {
    pthread_t thread;

    if(pthread_create(&thread, NULL, child_routine, NULL)){
        perror("pthread_create failed");
        return errno;
    }
    sleep(2);

    pthread_cancel(thread);
    void* res = NULL;
    int join = pthread_join(thread, &res);
    printf("join status: %d\n", join);
    if(res == PTHREAD_CANCELED) printf("thread cancelled\n");
    pthread_exit(NULL);
}
