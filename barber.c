//
//  main.c
//  barber
//
//  Created by Takashii on 2021/5/28.
//

#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <semaphore.h>
#include <stdlib.h>
#define CHAIRNUM 5

sem_t cus,bar; //sem
int empty_chair;
int working_status ;
int waiting_cus ;
pthread_mutex_t mutex;

void * Barber() {
    printf("no cus, barber sleep\n");
    while (1) {
        sem_wait(&cus);         //@@@@@@@@@@@@@@@@@@@
        pthread_mutex_lock(&mutex);
        empty_chair ++;
        waiting_cus --;
        pthread_mutex_unlock(&mutex);
        working_status =1;
        printf("working now....\n");
       // waiting_cus --;
        sleep(3);
        printf("work done\n");
        working_status =0;
        sem_post(&bar);       //@@@@@@@@@@@@@@@@@@@@@@@@
        if (waiting_cus ==0) {
            printf("no cus, barber sleep\n");
        }
    }
}

void * Customer (void* arg){
    int *temp = (int *)arg;
    int cus_no = *temp;
    pthread_mutex_lock(&mutex);
    if (empty_chair >0) {
        empty_chair --;
       // printf("%d 'st customer comes in\n", cus_no);
        sem_post(&cus);
        if(working_status ==0 && waiting_cus ==0) {
            printf("%d 'st customer ,wake up barber\n", cus_no);
            waiting_cus ++;
        }
        else {
            printf("%d 'st customer waits, now %d is waiting\n", cus_no, ++waiting_cus);
           // waiting_cus ++;
        }
        pthread_mutex_unlock(&mutex);
        sem_wait(&bar);
    }
    else {
        printf("%d 's customer leave for no seat\n", cus_no);
        pthread_mutex_unlock(&mutex);
    }
    
}


int main(int argc, const char * argv[]) {
    sem_init(&cus, 0, 0);
    sem_init(&bar, 0, 1);
    empty_chair = CHAIRNUM;
    working_status =0;
    waiting_cus = 0;
    pthread_t barber;
    pthread_t customer[CHAIRNUM * 10];
    pthread_mutex_init(&mutex, 0);

    
    pthread_create(&barber, NULL, Barber, NULL);
    for (int i=0; i<CHAIRNUM * 10; i++) {
        sleep(1);
        int temp_cus_id = i+1;
        pthread_create(&customer[i], NULL, Customer, &temp_cus_id);
    }
    
    pthread_join(barber, NULL);
    for (int i=0; i<CHAIRNUM * 10; i++) {
        pthread_join(customer[i], NULL);
    }
    
}
