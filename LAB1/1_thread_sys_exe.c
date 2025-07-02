#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <stdlib.h>

void *op(void *params) {
    pthread_t tid = pthread_self();
    printf("Inner thread: tid = %ld\n", tid); 
    execlp("./called", NULL); 
    pthread_exit(0); 
}

int main() {
    pid_t pid, pid1;

    pid = fork();
    if (pid < 0) {
        fprintf(stderr, "Fork Failed"); 
        return 1; 
    }
    else if (pid == 0) { 
        pid1 = getpid(); 
        printf("child: pid = %d\n", pid); 
        printf("child: pid1 = %d\n", pid1); 
        
        pthread_t tid; 
        pthread_attr_t attr; 

        pthread_attr_init(&attr); 

        pthread_create(&tid, &attr, op, NULL); 
        pthread_join(tid, NULL); 
    }
    else { 
        pid1 = getpid(); 
        printf("parent: pid = %d\n", pid);
        printf("parent: pid1 = %d\n", pid1); 
        wait(NULL); 
    }

    return 0; 
}

