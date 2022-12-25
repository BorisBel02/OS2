#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>

sem_t A, B, C, module;
void* constructA(){
    printf("hey\n");
    while(1) {
        sleep(1);
        sem_post(&A);
        //printf("A\n");
    }
}
void* constructB(){
    while(1){
        sleep(2);
        sem_post(&B);
        //printf("B\n");
    }
}
void* constructC(){
    while(1) {
        sleep(3);
        sem_post(&C);
        //printf("C\n");
    }
}
void* constructModule(){
    while(1) {
        sem_wait(&A);
        sem_wait(&B);
        sem_post(&module);
        printf("module\n");
    }
}
int main() {
    sem_init(&A, 1, 0);
    sem_init(&B, 1, 0);
    sem_init(&C, 1, 0);
    sem_init(&module, 1, 0);
    pthread_t threadA, threadB, threadC, modT;

    pthread_attr_t pthreadAttr;
    pthread_attr_setdetachstate(&pthreadAttr, PTHREAD_CREATE_DETACHED);

    pthread_create(&threadA, NULL, constructA, NULL);
    pthread_create(&threadB, NULL, constructB, NULL);
    pthread_create(&threadC, NULL, constructC, NULL);
    pthread_create(&modT, NULL, constructModule, NULL);
    while(1){
        sem_wait(&module);
        sem_wait(&C);
        printf("widget\n");
    }
    return 0;
}
