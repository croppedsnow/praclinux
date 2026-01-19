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

struct buf{
    long msgtype;
    char msgtext[256];
};

int main(int argc, char **argv){
    key_t key;
    int msgid, cycle = 0;
    int file, i = 0, flag_end = 1;
    char str[256], c;
    struct buf msg;

    key = ftok("/home/croppedsnow/prac/IPC/messager", 's');
    msgid = msgget(key, 0666 | IPC_CREAT | IPC_EXCL);

    file = open(argv[1], O_RDONLY);
    while(flag_end){
        i = 0;
        while(1){
            if(!read(file, msg.msgtext + i, 1)){
                flag_end = 0;
                break;
            }
            i++;
            if(msg.msgtext[i - 1] == '\n'){
                break;
            }

        }
        msg.msgtext[i] = '\0';
        //printf("----%s, %ld----\n", msg.msgtext, strlen(msg.msgtext));
        cycle = !cycle;
        msg.msgtype = cycle + 1;
        msgsnd(msgid, (struct msgbuf *) &msg, strlen(msg.msgtext) + 1, 0);
    }
    sleep(5);
    msgctl(msgid, IPC_RMID, NULL);
    printf("End server\n");
    return 0;
}