#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <unistd.h>

int main() {
    int i = 0;
    char str[100];
    fprintf(stderr, "Programm test2 begin\n");
    sleep(2);
    gets(str);
    printf("Come string: %s\n", str);
    fprintf(stderr, "Programm test2 finish\n");
    return 0;
}