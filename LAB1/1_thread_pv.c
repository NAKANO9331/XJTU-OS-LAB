#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

int shared_variable = 0;
sem_t semaphore; 
void *op1() {
    for (int i = 0; i < 5000; i++) {
        sem_wait(&semaphore); 
        shared_variable++;
        sem_post(&semaphore); 
    }
    return NULL;
}

void *op2() {
    for (int i = 0; i < 5000; i++) {
        sem_wait(&semaphore); 
        shared_variable += 2;
        sem_post(&semaphore); 
    }
    return NULL;
}

int main() {
    pthread_t thread1, thread2;

    sem_init(&semaphore, 0, 1);

    if (pthread_create(&thread1, NULL, op1, NULL) != 0) {
        perror("Failed to create thread1");
        exit(1);
    }

    if (pthread_create(&thread2, NULL, op2, NULL) != 0) {
        perror("Failed to create thread2");
        exit(1);
    }

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    printf("Final value of shared variable: %d\n", shared_variable);
    sem_destroy(&semaphore);

    return 0;
}

