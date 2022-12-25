//
// Created by boris on 11/12/22.
//

#include <stdio.h>
#include "TaskList.h"


void TaskList_init(struct taskList* tl, size_t size){
    pthread_mutexattr_t mutexattr;
    pthread_mutexattr_init(&mutexattr);
    pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_ERRORCHECK);
    pthread_mutex_init(&tl->taskList_mutex, &mutexattr);
    pthread_mutex_init(&tl->cond_mutex, &mutexattr);

    pthread_cond_init(&tl->task_present_notification, NULL);

    tl->node_qty = 0;
    tl->head = NULL;
    tl->last = NULL;

    tl->pending_tasks = (struct task**) malloc(sizeof(struct task*) * size);
    for (int i = 0; i < size; ++i) {
        tl->pending_tasks[i] = NULL;
    }
    pthread_mutexattr_destroy(&mutexattr);
}

struct task* createTask(enum taskType t, int task_socket){
    struct task* newtask = (struct task*)malloc(sizeof (struct task));
    if(newtask == NULL){
        return NULL;
    }
    newtask->type = t;
    newtask->next = NULL;
    newtask->client_sfd = task_socket;
    newtask->server_sfd = -1;

    return newtask;
}
struct task* getTask(struct taskList* tl){
    pthread_mutex_lock(&tl->taskList_mutex);
    if(tl->node_qty == 0){
        pthread_mutex_unlock(&tl->taskList_mutex);
        return NULL;
    }

    struct task* resTask = tl->head;
    tl->head = tl->head->next;
    --tl->node_qty;
    if(tl->node_qty == 0){
        tl->last = NULL;
    }
    pthread_mutex_unlock(&tl->taskList_mutex);
    return resTask;
}
int taskAppend(struct taskList* tl, struct task* apptask){
    if(apptask == NULL){
        return -1;
    }
    pthread_mutex_lock(&tl->taskList_mutex);
    if(tl->last != NULL){
        tl->last->next = apptask;
        tl->last = apptask;
    }
    else{
        tl->last = apptask;
        tl->head = apptask;
    }

    ++tl->node_qty;

    printf("tasks qty: %ld\nhead ptr: %p, last ptr: %p\n", tl->node_qty, tl->head, tl->last);
    pthread_cond_signal(&tl->task_present_notification);
    pthread_mutex_unlock(&tl->taskList_mutex);
    return 0;
}
int pendingTaskAppend(struct taskList* tl, struct task* apptask, int index){
    pthread_mutex_lock(&tl->taskList_mutex);
    tl->pending_tasks[index] = apptask;
    pthread_mutex_unlock(&tl->taskList_mutex);
    return 0;
}
int setOnExecution(struct taskList* tl, int index){
    taskAppend(tl, tl->pending_tasks[index]);
    tl->pending_tasks[index] = NULL;
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