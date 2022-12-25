//
// Created by boris on 25/12/22.
//

#include "write_response.h"

void* write_response(void* args){
    char err_msg[255];

    struct task* current_task = (struct task*)args;

    printf("Handling WRITE_RESPONSE\n");
    if(write(current_task->client_sfd, current_task->string, current_task->string_size) < -1){
        strerror_r(errno, err_msg, 255);
        printf("writing response failed.\nERROR: %s\n", err_msg);
    }
    close(current_task->client_sfd);
    free(current_task->string);
    printf("freeing current task\n");
    free(current_task);
}