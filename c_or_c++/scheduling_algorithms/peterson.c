#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>

#define N 2
bool flag[N] = {false};
int turn = 0;

void* process(void* arg) {
    int id = *((int *)arg);
    int other = 1 - id;
    while(true) {
        flag[id] = true;
        turn = other;

        while(flag[other] && turn == other){
            
        }

        printf("Process %d is in critical section\n", id);
        flag[id] = false;
        printf("Process %d is in remainder section\n", id);
    }
    return NULL;
}

int main() {
    pthread_t t1, t2;
    int ind1 = 0, ind2 = 1;
    pthread_create(&t1, NULL, process, &ind1);
    pthread_create(&t2, NULL, process, &ind2);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    return 0;
}