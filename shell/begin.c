#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

int main(int argc, char **argv){
    char c; int pid;
    signal(SIGINT, SIG_IGN);
    if((pid = fork()) == 0){
        execlp("gcc", "gcc", "-o", "shell", "shell.c", NULL);
    }else{
        if((pid = fork()) == 0){
            execlp("./shell", "./shell", NULL);
        }else{
            while(read(0, &c, 1))
                if(c == '$'){ 
                    kill(pid, SIGKILL);
                    break;
                }
        }
    }
    wait(NULL);
    return 0;
}