#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#define ITERATIONS 10000000

// структура для передачи потоку его порядкового номера и возвращения вычисленного им значения
typedef struct {
    double part_sum;
    int num;
} arg_struct_t;

int num_threads;

//ф-я потока. Прибавляет каждое n-ое число из ряда
void* calculate_pi(void* arg){
    arg_struct_t* arg_s = (arg_struct_t *)arg;

    arg_s->part_sum = 0;
    double del = (1 + (2 * arg_s->num));//знаменатель дроби из ряда

    for(int i = arg_s->num; i < ITERATIONS; i += num_threads){
        if(i%2 == 1){
            arg_s->part_sum -= 1/del;
        }
        else{
            arg_s->part_sum += 1/del;
        }
        del += 2 * num_threads;
    }

    pthread_exit(NULL);
}
int main(int argc, char* argv[]) {
    if(argc != 2){
        printf("bad input\n");
        exit(0);
    }

    char err_msg[256];

    double pi = 0;
    num_threads = atoi(argv[1]);

    pthread_t thread_list[num_threads];

    arg_struct_t* arg_struct_arr = (arg_struct_t*)malloc(sizeof(arg_struct_t) * num_threads);
    for(int i = 0; i < num_threads; ++i){
        arg_struct_arr[i].num = i;
    }

    int err;
    for(int i = 0; i < num_threads; ++i){//запуск потоков
        if((err = pthread_create(&thread_list[i], NULL, calculate_pi, (void*)&arg_struct_arr[i])) != 0){
            strerror_r(err, err_msg, 255);
            printf("pthread_create failed\nerror: %s\n", err_msg);
            free(arg_struct_arr);
            exit(err);
        }
    }
    for(int i = 0; i < num_threads; ++i){//завершение потоков
        if((err = pthread_join(thread_list[i], NULL)) != 0){
            strerror_r(err, err_msg, 255);
            printf("pthread_join failed\nerror %s\n", err_msg);
        }
        pi += arg_struct_arr[i].part_sum;//сложение возвращённых значений, чтобы получить оконательный ответ
    }

    printf("pi = %.15lf", pi * 4);//15 - кол-во симвлолов после запятой
    free(arg_struct_arr);
    return 0;
}
