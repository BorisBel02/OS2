#include <stdio.h>
#include <pthread.h>
#include <csignal>

void the_last_words(void* arg){
    printf("NOOOOOOO, I'M KILLED :(");
}
void* child_routine(void* args){
    pthread_cleanup_push(the_last_words, NULL);
    while(1) {
        printf("I AM STILL ALIVE!!!\n");
    }
    pthread_cleanup_pop(0);
}

int main() {
    pthread_t thread;

    pthread_create(&thread, NULL, child_routine, NULL);
    sleep(1);
    pthread_cancel(thread);
    void* res = NULL;
    int join = pthread_join(thread, &res);
    //printf("join status: %d\n", join);
    //if(res != NULL) printf("thread return: %s\n", (char *) res);
    pthread_exit(NULL);
}
