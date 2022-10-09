#include <stdio.h>
#include <pthread.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>

pthread_mutex_t condition_mutex;
char input_done = 0;
//pthread_cond_t input_done;

typedef struct arg{
    char* str;
    size_t len;
}arg;


void* sleep_sort(void* args){
    //pthread_cond_wait(&input_done, &condition_mutex);
    pthread_mutex_lock(&condition_mutex);
    pthread_mutex_unlock(&condition_mutex);
    arg* arg_struct = (arg*)args;
    sleep(arg_struct->len);
    printf("%s", arg_struct->str);
    free(arg_struct->str);
    free(arg_struct);
    pthread_exit(NULL);
}
int main() {
    pthread_mutex_init(&condition_mutex, NULL);
    pthread_cond_init(&input_done, NULL);

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);


    char* buf = malloc(sizeof (char) * BUFSIZ);
    size_t len = 0;
    pthread_mutex_lock(&condition_mutex);
    while(1){
        fgets(buf, BUFSIZ, stdin);
        if(buf[0] == '\n'){
            break;
        }

        len = strlen(buf);
        char* str = malloc(sizeof (char) * (len + 1));
        strcpy(str, buf);

        arg* arg_struct = (arg*)malloc(sizeof(arg));
        arg_struct->str = str;
        arg_struct->len = len;

        pthread_t new_thread;
        pthread_create(&new_thread, &attr, sleep_sort, (void*) arg_struct);
        //printf("thread create");
    }
    //pthread_cond_broadcast(&input_done);
    pthread_mutex_unlock(&condition_mutex);
    pthread_attr_destroy(&attr);
    free(buf);

    pthread_exit(NULL);
}
