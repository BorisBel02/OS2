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

char* strings[100];
void* sleep_sort(void* args){
    char* str = (char*)args;
    sleep(strlen(str));
    printf("%s", str);
    free(str);
    pthread_exit(NULL);
}
int main() {


    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);


    char* buf = malloc(sizeof (char) * BUFSIZ);
    size_t len = 0;
    pthread_mutex_lock(&condition_mutex);
    size_t counter = 0;
    while(counter < 100){
        fgets(buf, BUFSIZ, stdin);
        if(buf[0] == '\n'){
            break;
        }

        len = strlen(buf);
        char* str = malloc(sizeof (char) * (len + 1));
        strcpy(str, buf);

        strings[counter] = str;

        counter++;
        //printf("thread create");
    }
    for(int i = 0; i < counter; ++i){
        pthread_t new_thread;
        pthread_create(&new_thread, &attr, sleep_sort, (void*)strings[i]);
    }
    pthread_mutex_unlock(&condition_mutex);
    pthread_attr_destroy(&attr);
    free(buf);

    pthread_exit(NULL);
}
