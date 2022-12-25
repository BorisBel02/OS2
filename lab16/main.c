#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>

sem_t* sem0,* sem1;

void* child_thread(){
    for(int i = 0; i < 10; ++i){
        sem_wait(sem1);
        printf("%d child thread text!\n", i);
        sem_post(sem0);
    }
}
int main() {
    if((sem0 = sem_open("/sem0", O_CREAT, S_IRUSR | S_IWUSR, 1)) == SEM_FAILED){
        perror("sem_open failed");
        exit(1);
    }
    if((sem1 = sem_open("/sem1", O_CREAT, S_IRUSR | S_IWUSR, 0)) == SEM_FAILED){
        perror("sem_open failed");
        exit(1);
    }


    pid_t fork_retval;
    if((fork_retval = fork()) == -1){
        perror("fork failed");
    }
    else if(fork_retval == 0){
        child_thread();
    }
    else {
        for (int i = 0; i < 10; ++i) {
            sem_wait(sem0);
            printf("%d master thread text!\n", i);
            sem_post(sem1);
        }
        wait(0);
    }

    sem_close(sem0);
    sem_close(sem1);


    exit(0);
}
