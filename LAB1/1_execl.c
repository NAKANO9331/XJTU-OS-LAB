#include <stdio.h> 
#include <stdlib.h>
#include <pthread.h>

int shared_variable = 0; 
pthread_mutex_t lock; 

void *op1() {
    for (int i = 0; i < 5000; i++) { 
        pthread_mutex_lock(&lock); 
        shared_variable++; 
        pthread_mutex_unlock(&lock); 
    }
    return NULL; 
}

void *op2() {
    for (int i = 0; i < 5000; i++) { 
        pthread_mutex_lock(&lock); 
        shared_variable += 2; 
        pthread_mutex_unlock(&lock); 
    }
    return NULL; 
}

int main() {
    pthread_t thread1, thread2; 
    
    pthread_mutex_init(&lock, NULL);

    if (pthread_create(&thread1, NULL, op1, NULL) != 0) {
        perror("Failed to create thread1"); 
        exit(1); 

        
    if (pthread_create(&thread2, NULL, op2, NULL) != 0) {
        perror("Failed to create thread2");
        exit(1); 
    }

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    printf("Final value of shared variable: %d\n", shared_variable);

    pthread_mutex_destroy(&lock);

    return 0; 
}

