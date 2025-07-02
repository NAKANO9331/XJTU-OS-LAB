#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 

int pid1, pid2; 

int main() {
    int fd[2]; 
    char InPipe[10000] = {0}; 
    char c1 = '1', c2 = '2'; 
    pipe(fd); 
    
    while ((pid1 = fork()) == -1); 
    if (pid1 == 0) { 
        for (int i = 0; i < 2000; i++) {
            write(fd[1], &c1, 1);
            usleep(100); 
        }
        exit(0); 
    } 
    else { 

        while ((pid2 = fork()) == -1);
        if (pid2 == 0) { 
            for (int i = 0; i < 2000; i++) {
                write(fd[1], &c2, 1); 
                usleep(100); 
            } 
            exit(0); 
        } 
        else { 
            waitpid(pid1, NULL, 0); 
            waitpid(pid2, NULL, 0); 

            close(fd[1]); 
            
            read(fd[0], InPipe, 4000);    
            InPipe[4000] = '\0'; 
            printf("%s\n", InPipe); 

            close(fd[0]);
            exit(0); 
        } 
    }
}

