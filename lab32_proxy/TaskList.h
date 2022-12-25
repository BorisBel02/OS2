//
// Created by boris on 25/12/22.
//

#ifndef LAB33_PROXY_TASKLIST_H
#define LAB33_PROXY_TASKLIST_H

#include <pthread.h>
#include <stdlib.h>


enum taskType{
    REQUEST,
    WRITE_REQUEST,
    READ_RESPONSE,
    WRITE_RESPONSE
};

typedef struct task{
    enum taskType type;
    int client_sfd;
    int server_sfd;

    char* string;
    size_t string_size;

    char* cache_key;

    struct workThreadArguments* arguments;
}task;

typedef struct taskList{
    pthread_mutex_t taskList_mutex;

    struct task** pending_tasks;

}taskList;

void TaskList_init(struct taskList* tl, size_t size);

struct task* createTask(enum taskType t, int task_sock, struct workThreadArguments* threadargs);

int pendingTaskAppend(struct taskList* tl, struct task* apptask, int index);

int deleteTaskFromPendingTasks(struct taskList* tl, int index);

#endif //LAB33_PROXY_TASKLIST_H
