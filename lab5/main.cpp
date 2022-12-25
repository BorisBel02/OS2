#include <stdio.h>
#include <pthread.h>
#include <csignal>

void the_last_words(void* arg){
    printf("NOOOOOOO, I'M KILLED :(");
}
void* child_routine(void* args){
    pthread_cleanup_push(the_last_words, NULL);//положить в стек функции при завершении ф-ю 
    while(1) {
        printf("I AM STILL ALIVE!!!\n");
    }
    pthread_cleanup_pop(0);//вытащить из стека. Если аргумент != 0 то ещё и выполнить.
    //cleanup_push и cleanup_pop - макросы, в первом есть '{' во втором '}' => всегда нужно писать вместе иначе гг.
}

int main() {
    pthread_t thread;

    pthread_create(&thread, NULL, child_routine, NULL);
    sleep(1);
    pthread_cancel(thread);
    void* res = NULL;
    int join = pthread_join(thread, &res);
    pthread_exit(NULL);
}
