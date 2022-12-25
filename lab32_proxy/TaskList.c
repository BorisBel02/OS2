//
// Created by boris on 25/12/22.
//
#include <stdio.h>
#include "TaskList.h"


void TaskList_init(struct taskList* tl, size_t size){
    pthread_mutexattr_t mutexattr;
    pthread_mutexattr_init(&mutexattr);
    pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_ERRORCHECK);
    pthread_mutex_init(&tl->taskList_mutex, &mutexattr);

    tl->pending_tasks = (struct task**) malloc(sizeof(struct task*) * size);
    for (int i = 0; i < size; ++i) {
        tl->pending_tasks[i] = NULL;
    }
    pthread_mutexattr_destroy(&mutexattr);
}

struct task* createTask(enum taskType t, int task_socket, struct workThreadArguments* threadargs){
    struct task* newtask = (struct task*)malloc(sizeof (struct task));
    if(newtask == NULL){
        return NULL;
    }
    newtask->type = t;
    newtask->client_sfd = task_socket;
    newtask->server_sfd = -1;
    newtask->arguments = threadargs;
    return newtask;
}


int pendingTaskAppend(struct taskList* tl, struct task* apptask, int index){
    pthread_mutex_lock(&tl->taskList_mutex);
    tl->pending_tasks[index] = apptask;
    pthread_mutex_unlock(&tl->taskList_mutex);
    return 0;
}

int deleteTaskFromPendingTasks(struct taskList* tl, int index){
    struct task* t = tl->pending_tasks[index];
    free(t->string);
    free(t->cache_key);
    free(t);
    tl->pending_tasks[index] = NULL;
    return 0;
}