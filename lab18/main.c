#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>


typedef struct node{
    struct node* next;

    char* string;
    pthread_mutex_t node_mutex;
}node;
typedef struct list{
    size_t list_size;

    struct node* first;
    struct node* last;

}list;

pthread_t sort_thread;
pthread_mutex_t list_lock = PTHREAD_MUTEX_INITIALIZER;
struct list string_list;
char string[BUFSIZ];


void* sort(){
    char err_msg[256];
    for(;;){
        sleep(5);
        pthread_mutex_lock(&list_lock);
        if(string_list.list_size == 0){
            continue;
        }
        struct node* cmp_node = string_list.first;
        while(cmp_node->next != NULL){
            struct node* cur_node = cmp_node->next;
            while(cur_node != NULL){
                pthread_mutex_lock(&cur_node->node_mutex);
                if(strcmp(cmp_node->string, cur_node->string) > 0){
                    pthread_mutex_lock(&cmp_node->node_mutex);
                    char* strbuf = cur_node->string;
                    cur_node->string = cmp_node->string;
                    cmp_node->string = strbuf;
                    pthread_mutex_unlock(&cmp_node->node_mutex);
                }
                pthread_mutex_unlock(&cur_node->node_mutex);
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
    char err_msg[256];
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
    new_node->next = NULL;
    pthread_mutex_init(&new_node->node_mutex, NULL);

    if(string_list.list_size == 0){
        string_list.first = new_node;
        string_list.last = new_node;
    }
    else{
        pthread_mutex_t last_node_mutex = string_list.last->node_mutex;
        pthread_mutex_lock(&last_node_mutex);
        string_list.last->next = new_node;
        string_list.last = new_node;
        pthread_mutex_unlock(&last_node_mutex);
    }
    ++string_list.list_size;
}
void print_list(){
    struct node* current_node = string_list.first;
    while(current_node != NULL){
        pthread_mutex_lock(&current_node->node_mutex);
        printf("%s\n", current_node->string);
        pthread_mutex_unlock(&current_node->node_mutex);
        current_node = current_node->next;
    }
}
int main() {
    char err_msg[256];

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
