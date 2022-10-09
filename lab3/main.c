#include <stdio.h>
#include <pthread.h>


void* print_lines(void* arg){
    char* text = (char*)arg;
    if(printf("%s\n", text) == -1){
        perror("printf failed");
    }
    pthread_exit(NULL);
}
int main() {
    pthread_t t0, t1, t2, t3;

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    char* text0 = "NAME\n   nptl - Native POSIX Threads Library\n";
    char* text1 = "DESCRIPTION\n    NPTL  (Native  POSIX  Threads  Library) is the GNU C library POSIX threads implementation that is used on modern Linux systems.";
    char* text2 = "   NPTL and signals\n"
                  "       NPTL makes internal use of the first two real-time signals (signal numbers 32 and  33).   One  of\n"
                  "       these  signals is used to support thread cancellation and POSIX timers (see timer_create(2)); the\n"
                  "       other is used as part of a mechanism that ensures all threads in a process always have  the  same\n"
                  "       UIDs and GIDs, as required by POSIX.  These signals cannot be used in applications.";
    char* text3 = "   NPTL and process credential changes\n"
                  "       At the Linux kernel level, credentials (user and group IDs) are a per-thread attribute.  However,\n"
                  "       POSIX requires that all of the POSIX threads in a process have the same credentials.  To accommo‐\n"
                  "       date this requirement, the NPTL implementation wraps all of the system calls that change  process\n"
                  "       credentials  with functions that, in addition to invoking the underlying system call, arrange for\n"
                  "       all other threads in the process to also change their credentials.";


    if(pthread_create(&t0, &attr, print_lines, (void*)text0) != 0){
        perror("pthread_create t0");
    }
    if(pthread_create(&t1, &attr, print_lines, (void*)text0) != 1){
        perror("pthread_create t1");
    }
    if(pthread_create(&t2, &attr, print_lines, (void*)text0) != 2){
        perror("pthread_create t2");
    }
    if(pthread_create(&t3, &attr, print_lines, (void*)text0) != 3){
        perror("pthread_create t3");
    }

    pthread_attr_destroy(&attr);

    pthread_exit(NULL);
}
