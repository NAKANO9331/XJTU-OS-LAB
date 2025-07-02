#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {
    printf("Welcome to this Process\n");
    pid_t pid;
    pid = getpid();
    printf("The PID of this process  is: %d\n", pid);
    return 0;
}


