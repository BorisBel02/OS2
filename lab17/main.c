#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>


typedef struct node{
    struct node* next;

    char* string;
}node;
typedef struct list{
    size_t list_size;

    struct node* first;
    struct node* last;

}list;

pthread_t sort_thread;
pthread_mutex_t list_lock = PTHREAD_MUTEX_INITIALIZER;
struct list string_list;
char err_msg[256];
char string[BUFSIZ];


void* sort(){
    for(;;){
        sleep(1);
        if(string_list.list_size <= 1){
            continue;
        }
        pthread_mutex_lock(&list_lock);
        struct node* cmp_node = string_list.first;
        while(cmp_node->next != NULL){
            struct node* cur_node = cmp_node->next;
            while(cur_node != NULL){
                if(strcmp(cmp_node->string, cur_node->string) > 0){
                    char* strbuf = cur_node->string;
                    cur_node->string = cmp_node->string;
                    cmp_node->string = strbuf;
                }
                cur_node = cur_node->next;
            }
            cmp_node = cmp_node->next;
        }
        pthread_mutex_unlock(&list_lock);
    }
}
void list_free(){
    pthread_mutex_lock(&list_lock);
    pthread_cancel(sort_thread);
    struct node* current_node = string_list.first;
    while(current_node != NULL){
        struct node* next = current_node->next;
        free(current_node->string);
        free(current_node);
        current_node = next;
    }
    pthread_mutex_unlock(&list_lock);
}
void create_node(char* src, size_t size){
    struct node* new_node = (struct node*)malloc(sizeof (struct node));
    if(new_node == NULL){
        strerror_r(errno, err_msg, 255);
        printf("failed to allocate memory for the new node. Error: %s\n", err_msg);
        list_free();
        exit(EXIT_FAILURE);
    }
    new_node->string = malloc(sizeof (char) * size - 1);
    if(new_node->string == NULL){
        strerror_r(errno, err_msg, 255);
        printf("failed to allocate memory for the new string. Error: %s\n", err_msg);
        list_free();
        free(new_node);
        exit(EXIT_FAILURE);
    }
    memcpy(new_node->string, src, size*sizeof (char));
    new_node->string[size - 1] = '\0';

    pthread_mutex_lock(&list_lock);
    if(string_list.list_size == 0){
        string_list.first = new_node;
        string_list.last = new_node;
    }
    else{
        string_list.last->next = new_node;
        string_list.last = new_node;
    }
    new_node->next = NULL;
    ++string_list.list_size;
    pthread_mutex_unlock(&list_lock);
}
void print_list(){
    pthread_mutex_lock(&list_lock);
    struct node* current_node = string_list.first;
    while(current_node != NULL){
        printf("%s\n", current_node->string);
        current_node = current_node->next;
    }
    pthread_mutex_unlock(&list_lock);
}
int main() {

    int err;
    if((err = pthread_create(&sort_thread, NULL, sort, NULL)) != 0){
        strerror_r(err, err_msg, 255);
        printf("failed to create thread. Error: %s\n", err_msg);
        pthread_exit(NULL);
    }

    ssize_t strlen;
    for(;;){
        if((strlen = read(0, string, BUFSIZ)) == -1){
            strerror_r(errno, err_msg, 255);
            printf("read failed. Error: %s\n", err_msg);
            list_free();
            exit(EXIT_FAILURE);
        }
        if(strlen == 0){
            break;
        }
        if(string[0] == '\n'){
            print_list();
            continue;
        }

        size_t pos = 0;
        for(int i = 0; i < strlen/80; ++i){
            create_node(&string[pos], 80);
            pos += 80;
        }
        if(strlen % 80){
            create_node(&string[pos], strlen % 80);
        }

    }
    list_free();
    pthread_join(sort_thread, NULL);
    exit(EXIT_SUCCESS);
}
