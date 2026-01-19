#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>

struct buf{
    long msgtype;
    char msgtext[256];
};

int main(int argc, char **argv){ // В первом аргументе файл в который записывать, во втором 1 или 2, в два клиента разные числа
    key_t key_msg, key_sem;
    int msgid, msgtype, semid, count = 0;
    int file, i = 0;
    struct buf msg;
    struct sembuf semup = {0, 1, 0}, semdown = {0, -2, 0};
    char str[100];

    sleep(2);
    msgtype = atoi(argv[2]);

    key_msg = ftok("/home/croppedsnow/prac/IPC/messager", 's');
    key_sem = ftok("/home/croppedsnow/prac/IPC/semaphore", 't');
    msgid = msgget(key_msg, 0);
    semid = semget(key_sem, 1, 0666 | IPC_CREAT);

    file = open(argv[1], O_RDWR, 0);

    semop(semid, &semup, 1);
    semup.sem_op++;

    for(;;){
        
        if(msgrcv(msgid, (struct msgbuf*) (&msg), 256, msgtype, MSG_NOERROR) == -1)
            break;

        semop(semid, &semdown, 1);
        lseek(file, 0, 2);
        //printf("****%s****\n", msg.msgtext);
        sprintf(str, "%#x", getpid());
        write(file, str, strlen(str));
        write(file, msg.msgtext, strlen(msg.msgtext));
        semop(semid, &semup, 1);
    }
    fprintf(stderr, "End client %#x\n", getpid());
    sleep(1);
    semctl(semid, 0, IPC_RMID);
    return 0;
}