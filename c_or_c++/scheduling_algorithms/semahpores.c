#include <stdio.h>
#include <Windows.h>
#include <pthread.h>

typedef struct
{
    pthread_mutex_t mutex;
    pthread_cond_t condition;
    int value;
} semaphores;

void semaphores_init(semaphores* sem, int init) {
    pthread_mutex_init(&(sem->mutex), NULL);
    pthread_cond_init(&(sem->condition), NULL);
    sem->value = init;
}

void semaphore_wait(semaphores* sem) {
    pthread_mutex_lock(&(sem->mutex));
    printf("mutex lock in semaphore wait fn\n");
    while(sem->value <= 0) {
        pthread_cond_wait(&(sem->condition), &(sem->mutex));
    }
    sem->value--;
    pthread_mutex_unlock(&(sem->mutex));
    printf("mutex unlock in semaphore wait fn\n");
}

void semaphore_signal(semaphores* sem) {
    pthread_mutex_lock(&(sem->mutex));
    printf("mutex lock in semaphore signal fn\n");
    sem->value++;
    pthread_cond_signal(&(sem->condition));
    pthread_mutex_unlock(&(sem->mutex));
    printf("mutex unlock in semaphore signal fn\n");
}

void* thread_function(void* arg) {
    semaphores *sem = (semaphores *)arg;
    printf("Thread waiting\n");
    semaphore_wait(sem);

    printf("Thread acquired the semaphore\n");
    for (int i = 0; i < 10;i++) {
        printf("#\n");
        // Sleep(1);
    }
    
    printf("Thread releasing the semaphore\n");
    semaphore_signal(sem);
    
    return NULL;
}

int main() {
    semaphores sem;
    semaphores_init(&sem, 3);
    
    pthread_t thread1, thread2, thread3, thread4, thread5 ;
    pthread_create(&thread1, NULL, thread_function, (void *)&sem);
    pthread_create(&thread2, NULL, thread_function, (void *)&sem);
    pthread_create(&thread3, NULL, thread_function, (void *)&sem);
    pthread_create(&thread4, NULL, thread_function, (void *)&sem);
    pthread_create(&thread5, NULL, thread_function, (void *)&sem);
    
    printf("Main thread performing some work\n");
    
    printf("Main thread waiting\n");
    semaphore_wait(&sem);
    
    printf("Main thread acquired the semaphore\n");
    
    for (int i = 0; i < 10;i++) {
        printf(".\n");
        // Sleep(1);
    }
    
    printf("Main thread releasing the semaphore\n");
    semaphore_signal(&sem);

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    pthread_join(thread3, NULL);
    pthread_join(thread4, NULL);
    pthread_join(thread5, NULL);

    return 0;
}