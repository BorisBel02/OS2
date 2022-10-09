#include <stdio.h>
#include <pthread.h>
#include <errno.h>

void* child_thread(){
    for(int i = 0; i < 10; ++i){
        printf("child thread text!\n");
    }

    pthread_exit(NULL);
}
int main() {
    pthread_t thread;

    if(pthread_create(&thread, NULL, child_thread, NULL) != 0){
        perror("pthread_create failed");
        return errno;
    }
    
    void* res = NULL;
    if(pthread_join(thread, &res) != 0){
        perror("join failed");
    }
    for(int i = 0; i < 10; ++i){
        printf("master thread text!\n");
    }
    printf("%s", (char*)res);

    pthread_exit(NULL);
}
