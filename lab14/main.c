#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <semaphore.h>

sem_t sem0, sem1;
void* child_thread(){
    for(int i = 0; i < 10; ++i){
        sem_wait(&sem1);
        printf("%d child thread text!\n", i);
        sem_post(&sem0);
    }
    pthread_exit(NULL);
}
int main() {
    pthread_t thread;

    sem_init(&sem0, 1, 1);
    sem_init(&sem1, 1, 0);

    if(pthread_create(&thread, NULL, child_thread, NULL) != 0){
        perror("pthread_create failed");
        return errno;
    }


    for(int i = 0; i < 10; ++i){
        sem_wait(&sem0);
        printf("%d master thread text!\n", i);
        sem_post(&sem1);
    }

    sem_destroy(&sem0);
    sem_destroy(&sem1);

    void* res = NULL;
    if(pthread_join(thread, &res) != 0){
        perror("join failed");
    }
    printf("%s", (char*)res);

    pthread_exit(NULL);
}
