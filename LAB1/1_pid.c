#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int global = 0; 

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
        global += 100;
        printf("child: global = %d\n", global);
        printf("child: global_address = %x\n", &global); 
    }
    else { 
        pid1 = getpid(); 
        printf("parent: pid = %d\n", pid); 
        printf("parent: pid1 = %d\n", pid1); 
        global += 200; 
        printf("parent: global = %d\n", global); 
        printf("parent: global_address = %x\n", &global); 
        wait(NULL); 
    }

    global += 1; 
    printf("final: global = %d\n", global); 

    return 0; 
}


