//
//  main.c
//  rw
//
//  Created by Takashii on 2021/5/29.
//
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>

#define SEMKEY 1234
#define BUF_LENGTH (sizeof(struct buf))
#define SHM_MODE 0600
#define NEED_R 4//reader数量
#define NEED_W 2//writer数量
#define WORKS_W 4//write次数
#define WORKS_R 6//read次数
#define COUNT_MUTEX 0//在信号集里的编号
#define RW_MUTEX 1//在信号集里的编号
#define EQUAL_MUTEX 2//在信号集里的编号

struct buf
{
    char text[100]; // 记录写入 和 读取 的文本
    
};
//random time
int get_random(){
    int t = rand()%4;
    return t;
}
//p
void p(int sem_id, int sem_num)
{
    struct sembuf xx;
    xx.sem_num = sem_num;//信号量编号
    xx.sem_op = -1;//P
    xx.sem_flg = 0;
    semop(sem_id, &xx, 1);
}

//V操作
void v(int sem_id, int sem_num)
{
    struct sembuf xx;
    xx.sem_num = sem_num;//信号量编号
    xx.sem_op = 1;//V
    xx.sem_flg = 0;
    semop(sem_id, &xx, 1);
}

int main(int argc, const char * argv[]) {
    //--------------mutex  -------------
    int count = 0;
  /*  int count_mutex;
    int rw_mutex;
    int equal_mutex;*/
    int sem_id;
    //--------------share_memory  -------
    int shm_id;
    //----------------process & time init  -------
    time_t now;
    pid_t pid_r, pid_w;
    //--------------mutex init -------------
    sem_id = semget(SEMKEY, 3, IPC_CREAT | 0660);//create 3 sem in 1 semset
    struct buf * shmptr;                        //shmptr
    if (sem_id >= 0)
    {
        printf("Main process starts. Semaphore created.\n");
    }
    
    semctl(sem_id, 0, SETVAL, 1);//count_mutex
    semctl(sem_id, 1, SETVAL, 1);//rw_mutex;
    semctl(sem_id, 2, SETVAL, 1);//equal_mutex;
    //-----------share memory init--------
    //创建新的共享内存
    if ((shm_id = shmget(IPC_PRIVATE, BUF_LENGTH, SHM_MODE)) < 0)
    {
        printf("Error on shmget.\n");
        exit(1);
    }
    //把共享内存区对象映射到调用进程的地址空间
    if ((shmptr = shmat(shm_id, 0, 0)) == (void *)-1)
    {
        printf("Error on shmat.\n");
        exit(1);
    }

    //--------------create process ---------------
    int num_r,num_w;
    num_r = num_w = 0;
    
    while ((num_w++) < NEED_W) {
        if ((pid_w = fork()) < 0)
        {
            printf("Error on fork.\n");
            exit(1);
        }
        //is children process, create writer
        if (pid_w == 0) {
            //将共享内存区对象映射到调用进程的地址空间
            if ((shmptr = shmat(shm_id, 0, 0)) == (void *)-1)
            {
                printf("Error on shmat.\n");
                exit(1);
            }
            // begin to write
            for (int i = 0; i < WORKS_W; i++) {
            //    p(sem_id, EQUAL_MUTEX);
                p(sem_id, RW_MUTEX);
                sleep(get_random());//等待随机时间
                now = time(NULL);
                printf("%02d:%02d:%02d\t", localtime(&now)->tm_hour, localtime(&now)->tm_min, localtime(&now)->tm_sec);
                printf("writer %d ",num_w);
                printf("write in hello%d\n", i+1);
                strcpy(shmptr->text, "hello");
                v(sem_id, RW_MUTEX);
             //   v(sem_id, EQUAL_MUTEX);
            }
            shmdt(shmptr);//断开共享内存连接
            exit(0);
            
        }
    }
    
    while ((num_r ++) < NEED_R) {
        if ((pid_r = fork()) < 0)
        {
            printf("Error on fork.\n");
            exit(1);
        }
        //is children process, create reader
        if (pid_r == 0) {
            //将共享内存区对象映射到调用进程的地址空间
            if ((shmptr = shmat(shm_id, 0, 0)) == (void *)-1)
            {
                printf("Error on shmat.\n");
                exit(1);
            }
            // begin to read
            for (int i = 0; i < WORKS_R; i++) {
              //  p(sem_id, EQUAL_MUTEX);
               // sleep(get_random());//等待随机时间
                p(sem_id, COUNT_MUTEX);
                if (count == 0) {
                    p(sem_id, RW_MUTEX);
                }
                count ++;
                v(sem_id, COUNT_MUTEX);
             //   v(sem_id, EQUAL_MUTEX);
                now = time(NULL);
                printf("%02d:%02d:%02d\t", localtime(&now)->tm_hour, localtime(&now)->tm_min, localtime(&now)->tm_sec);
                printf("count: %d  ", count);
                printf("reader %d ",num_r);
                printf("read text = %s%d \n", shmptr ->text, i+1);
                
                p(sem_id, COUNT_MUTEX);
                count --;
                printf("count_mutex count :%d \n", count);
                if (count == 0) {
                    v(sem_id, RW_MUTEX);
                    printf("rw_mutex count :%d \n", count);
                }
                v(sem_id, COUNT_MUTEX);
            }
            shmdt(shmptr);//断开共享内存连接
            exit(0);
        }
    }
    //主控程序最后退出
    while(wait(0) != -1);
    shmdt(shmptr);
    shmctl(shm_id, IPC_RMID, 0);
    semctl(sem_id, IPC_RMID, 0);
    printf("Main process ends.\n");
    fflush(stdout);
    exit(0);
    
}
