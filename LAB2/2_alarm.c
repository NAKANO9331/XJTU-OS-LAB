#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <signal.h>

pid_t pid1 = -1, pid2 = -1;

void handle_parent_interrupt(int sig) {
    if (pid1 <= 0 || pid2 <= 0) return; 
    printf("\nReceived soft interrupt\n");
    kill(pid1, 16);  
    printf("Sent 16 signal successfully\n");
    kill(pid2, 17);
    printf("Sent 17 signal successfully\n");
}

void handle_child1_signal(int sig) {
    printf("Child process 1 is killed by parent!!\n");
    exit(0); 
}

void handle_child2_signal(int sig) {
    printf("Child process 2 is killed by parent!!\n");
    exit(0); 
}

void handle_alarm(int sig) {
    printf("\nAlarm signal received, terminating processes...\n");
    if (pid1 > 0) kill(pid1, 16);
    if (pid2 > 0) kill(pid2, 17);
}

int main() {
    signal(SIGINT, handle_parent_interrupt);   
    signal(SIGQUIT, handle_parent_interrupt); 
    signal(SIGALRM, handle_alarm);             
    
    while (pid1 == -1) pid1 = fork(); 
    if (pid1 > 0) {
        
        while (pid2 == -1) pid2 = fork(); 
        if (pid2 > 0) { 
            printf("Start waiting for 5 seconds...\n");
            alarm(5);
            pause(); 
            printf("\nParent process is killed!!\n");
        } else { 
            signal(17, handle_child2_signal);
            printf("Child process 2 is ready\n");
            pause(); 
        }
    } else { 
        signal(16, handle_child1_signal);
        printf("Child process 1 is ready\n");
        pause(); 
    }
    
    return 0;
}

