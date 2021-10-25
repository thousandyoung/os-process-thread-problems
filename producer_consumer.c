//
//  main.c
//  pandc
//
//  Created by Takashii on 2021/5/27.
//
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>

#define NEED_P 2//生产者数量
#define NEED_C 3//消费者数量
#define WORKS_P 6//生产次数
#define WORKS_C 4//消费次数

#define BUF_LENGTH (sizeof(struct buf))
#define LETTER_NUM 3
#define SHM_MODE 0600

#define SEM_ALL_KEY 1234
#define SEM_EMPTY 0
#define SEM_FULL 1

//缓冲区结构（循环队列）
struct buf
{
    char letter[LETTER_NUM];//存放字符数组
    int head;//头指针
    int tail;//尾指针
    int is_empty;//空标志
};
//random time
int get_random(){
    int t = rand()%4;
    return t;
}

//a-z
char get_letter()
{
    char a;
    a = (char)((char)(rand() % 26) + 'a');
    return a;
}
//A-Z
char GET_LETTER()
{
    char a;
    a = (char)((char)(rand() % 26) + 'A');
    return a;
}

//P操作
void p(int sem_id, int sem_num)
{
    struct sembuf xx;
    xx.sem_num = sem_num;//操作信号在信号集中的编号，第一个信号的编号是0
    xx.sem_op = -1;//获取资源的使用权
    xx.sem_flg = 0;
    semop(sem_id, &xx, 1);
}

//V操作
void v(int sem_id, int sem_num)
{
    struct sembuf xx;
    xx.sem_num = sem_num;//操作信号在信号集中的编号，第一个信号的编号是0
    xx.sem_op = 1;//获取资源的使用权
    xx.sem_flg = 0;
    semop(sem_id, &xx, 1);
}

//主函数
int main(int argc, char * argv[])
{
    int i, j;
    int shm_id, sem_id;
    int num_p = 0, num_c = 0;
    //定义一个缓冲区指针shmptr
    struct buf * shmptr;
    char lt;
    time_t now;
    pid_t pid_p, pid_c;
    //创建&初始化两个新信号量
    sem_id = semget(SEM_ALL_KEY, 2, IPC_CREAT | 0660);
    if (sem_id >= 0)
    {
        printf("Main process starts. Semaphore created.\n");
    }
    semctl(sem_id, SEM_EMPTY, SETVAL, LETTER_NUM);
    semctl(sem_id, SEM_FULL, SETVAL, 0);
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
    shmptr->head = 0;
    shmptr->tail = 0;
    shmptr->is_empty = 1;
    
    while ((num_p++) < NEED_P)
    {
        if ((pid_p = fork()) < 0)
        {
            printf("Error on fork.\n");
            exit(1);
        }
        //如果是子进程，开始创建生产者
        if (pid_p == 0)
        {
            //将共享内存区对象映射到调用进程的地址空间
            if ((shmptr = shmat(shm_id, 0, 0)) == (void *)-1)
            {
                printf("Error on shmat.\n");
                exit(1);
            }
            for (i = 0; i < WORKS_P; i++)
            {
                p(sem_id, SEM_EMPTY);//对资源进行p操作
                
                
                sleep(get_random());//等待随机时间
                if (num_p ==1){
                    shmptr->letter[shmptr->tail] = lt = get_letter();//随机生成一个字母放入缓冲区
                }
                else
                if (num_p ==2){
                    shmptr->letter[shmptr->tail] = lt = GET_LETTER();//随机生成一个字母放入缓冲区
                }
                shmptr->tail = (shmptr->tail + 1) % LETTER_NUM;//尾指针后移
                shmptr->is_empty = 0;//空标志置0
                now = time(NULL);
                printf("%02d:%02d:%02d\t", localtime(&now)->tm_hour, localtime(&now)->tm_min, localtime(&now)->tm_sec);
                //输出缓冲区字符
                for (j = (shmptr->tail - 1 >= shmptr->head) ? (shmptr->tail - 1) : (shmptr->tail - 1 + LETTER_NUM); !(shmptr->is_empty) && j >= shmptr->head; j--)
                {
                    printf("%c", shmptr->letter[j % LETTER_NUM]);
                }
                printf("\tProducer %d puts '%c'.\n", num_p, lt);
                fflush(stdout);
                
                
                v(sem_id, SEM_FULL);//对资源进行v操作
            }
            shmdt(shmptr);//断开共享内存连接
            exit(0);
        }
    }
    
    while (num_c++ < NEED_C)
    {
        if ((pid_c = fork()) < 0)
        {
            printf("Error on fork.\n");
            exit(1);
        }
        //如果是子进程，开始创建消费者
        if (pid_c == 0)
        {
            if ((shmptr = shmat(shm_id, 0, 0)) == (void *)-1)
            {
                printf("Error on shmat.\n");
                exit(1);
            }
            for (i = 0; i < WORKS_C; i++)
            {
                p(sem_id, SEM_FULL);//对资源进行p操作
                
                
                sleep(get_random());//等待随机时间
                lt = shmptr->letter[shmptr->head];//取出缓冲区第一个字符
                shmptr->head = (shmptr->head + 1) % LETTER_NUM;//头指针后移
                shmptr->is_empty = (shmptr->head == shmptr->tail);
                now = time(NULL);
                printf("%02d:%02d:%02d\t", localtime(&now)->tm_hour, localtime(&now)->tm_min, localtime(&now)->tm_sec);
                //输出缓冲区字符
                for (j = (shmptr->tail - 1 >= shmptr->head) ? (shmptr->tail - 1) : (shmptr->tail - 1 + LETTER_NUM); !(shmptr->is_empty) && j >= shmptr->head; j--)
                {
                    printf("%c", shmptr->letter[j % LETTER_NUM]);
                }
                printf("\tConsumer %d gets '%c'.\n", num_c, lt);
                fflush(stdout);
                
                
                v(sem_id, SEM_EMPTY);//对资源进行v操作
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

