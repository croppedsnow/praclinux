#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <unistd.h>

int main(int argc, char **argv) {
    int i = 0;
    sleep(1);
    printf("%s\n", argv[1]);
    fprintf(stderr, "Programm test3 finish\n");
    return 1;
}