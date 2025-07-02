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

int main() {
    signal(SIGINT, handle_parent_interrupt);   
    signal(SIGQUIT, handle_parent_interrupt); 
    
    while (pid1 == -1) pid1 = fork(); 
    if (pid1 > 0) { 
        
        while (pid2 == -1) pid2 = fork(); 
        if (pid2 > 0) { 
            printf("Start waiting for 5 seconds...\n");
            sleep(5);
            while (wait(NULL) != -1); 
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
