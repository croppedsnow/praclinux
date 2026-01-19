#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/file.h>

/*
    Не обработаны неправильные строки
    Нельзя использовать () > file
    Фоновый режим на все процессы и только в конце строки
*/

struct tree_proc{ // Дерево операций над процессами
    struct tree_proc *left;
    struct tree_proc *right;
    char **prog; // Название программы со всеми аргументами или название файла для чтения/записи
    char *oper; // Операция для процессов
} tree_default = {NULL, NULL, NULL, NULL};

typedef struct tree_proc tree_proc;

void conveer(tree_proc *);
void consistly(tree_proc *, int, int);

int choose_func_oper(tree_proc *oper_node){ // Выбирает функцию по операции, возвращает дескриптор файла или 0
    int file_desc = 0;

    if(!strcmp(oper_node->oper, "|")){

        conveer(oper_node);

    }else if(!strcmp(oper_node->oper, "&&")){

        consistly(oper_node, 1, 0);

    }else if(!strcmp(oper_node->oper, "||")){

        consistly(oper_node, 0, 0);

    }else if(!strcmp(oper_node->oper, ";")){

        consistly(oper_node, 0, 1);

    }else if(!strcmp(oper_node->oper, ">")){

        file_desc = open(oper_node->prog[0], O_WRONLY | O_CREAT | O_TRUNC, 0777);

    }else if(!strcmp(oper_node->oper, ">>")){

        file_desc = open(oper_node->prog[0], O_RDWR | O_CREAT, 0777);
        lseek(file_desc, 0, 2);

    }else if(!strcmp(oper_node->oper, "<")){

        file_desc = open(oper_node->prog[0], O_RDONLY, 0);

    }

    return file_desc;
}

void conveer(tree_proc *oper_node){                     // Конвейер 
    int pipe_desc[2], file_desc = 0, err1 = 0, err2 = 0;
    pipe(pipe_desc);

    if(fork() == 0){
        close(pipe_desc[0]);
        dup2(pipe_desc[1], 1);
        close(pipe_desc[1]);

        if(oper_node->left->oper != NULL){
            choose_func_oper(oper_node->left);
        }else{

            if(oper_node->left->left != NULL){
                file_desc = choose_func_oper(oper_node->left->left);
                dup2(file_desc, 0);
                close(file_desc);
            }

            if(oper_node->left->right != NULL){
                fprintf(stdout, "Error: Can't write to file 1 process in conveyor\n");
            }

            execvp(oper_node->left->prog[0], oper_node->left->prog);
        }
    }

    if(fork() == 0){
        close(pipe_desc[1]);
        dup2(pipe_desc[0], 0);
        close(pipe_desc[0]);

        if(oper_node->right->oper != NULL){
            choose_func_oper(oper_node->right);
        }else{

            if(oper_node->right->right != NULL){
                file_desc = choose_func_oper(oper_node->right->right);
                dup2(file_desc, 1);
                close(file_desc);
            }

            if(oper_node->right->left != NULL){
                fprintf(stdout, "Error: Can't read from file 2 process in conveyor\n");
            }

            execvp(oper_node->right->prog[0], oper_node->right->prog);
        }
    }
    
    close(pipe_desc[0]);
    close(pipe_desc[1]);
    wait(&err1);
    wait(&err2);
    exit((err2 & 0xFF00) >> 8);
}

void consistly(tree_proc *oper_node, int flag_condition, int flag_without_condition){ // Выполнение последовательно
    int file_desc = 0, err1 = 0, err2 = 0;
    if(fork() == 0){

        if(oper_node->left->oper != NULL){
            choose_func_oper(oper_node->left);
        }else{

            if(oper_node->left->left != NULL){
                file_desc = choose_func_oper(oper_node->left->left);
                dup2(file_desc, 0);
                close(file_desc);
            }

            if(oper_node->left->right != NULL){
                file_desc = choose_func_oper(oper_node->left->right);
                dup2(file_desc, 1);
                close(file_desc);
            }

            execvp(oper_node->left->prog[0], oper_node->left->prog);
        }
    }

    wait(&err1);
    if((!(err1 & 0xFFFF) == !flag_condition) && !flag_without_condition){ // Выбор, выполнять ли следующую программу
        exit((err1 & 0xFF00) >> 8);
    }

    if(fork() == 0){

        if(oper_node->right->oper != NULL){
            choose_func_oper(oper_node->right);
        }else{

            if(oper_node->right->left != NULL){
                file_desc = choose_func_oper(oper_node->right->left);
                dup2(file_desc, 0);
                close(file_desc);
            }

            if(oper_node->right->right != NULL){
                file_desc = choose_func_oper(oper_node->right->right);
                dup2(file_desc, 1);
                close(file_desc);
            }

            execvp(oper_node->right->prog[0], oper_node->right->prog);
        }
    }
    
    wait(&err2);
    exit((err2 & 0xFF00) >> 8);
}

void background(){ // Фоновый режим
    int file_null;
    signal(SIGINT, SIG_IGN);
    file_null = open("/dev/null", O_RDWR, 0);
    dup2(file_null, 1);
    dup2(file_null, 0);
    close(file_null);
}

#define MAX_ARG 100
#define MAX_LEN_ARG 1000
#define MAX_LEN_STR 100000

struct stack_oper{
    char *oper;
    struct stack_oper *next;
};
typedef struct stack_oper stack_oper;

struct stack_tree{
    tree_proc *node;
    struct stack_tree *next;
};
typedef struct stack_tree stack_tree;

void push_oper(stack_oper **stack_proc, char *oper){ // Положить в стек операций
    stack_oper *stack_new;
    stack_new = (stack_oper *) malloc (sizeof(stack_oper));
    stack_new->oper = (char *) malloc (sizeof(char) * 3);
    strcpy(stack_new->oper, oper);
    stack_new->next = *stack_proc;
    *stack_proc = stack_new;
}

void pop_oper(stack_oper **stack_proc){ // Убрать из стека операций
    stack_oper *stack_ret;
    stack_ret = (*stack_proc)->next;
    free((*stack_proc)->oper);
    free((*stack_proc));
    *stack_proc = stack_ret;
}

void push_tree(stack_tree **stack_node, tree_proc* tree_node){ // Положить в стек вершин дерева
    stack_tree *stack_new;
    stack_new = (stack_tree *) malloc (sizeof(stack_tree));
    stack_new->node = tree_node;
    stack_new->next = *stack_node;
    *stack_node = stack_new;
}

void pop_tree(stack_tree **stack_node){ // Убрать из стека вершин дерева
    stack_tree *stack_ret;
    stack_ret = (*stack_node)->next;
    free(*stack_node);
    *stack_node = stack_ret;
}

// Сброс из массива аргументов в нужный узел дерева
void mas_arg_processing(stack_oper **stack_proc, stack_tree *stack_node, char (*mas_arg)[MAX_LEN_ARG], int *count_arg){
    tree_proc *now_tree;
    int index_for;

    if(*count_arg != 0){
        if((*stack_proc != NULL) && (((*stack_proc)->oper[0] == '<') || ((*stack_proc)->oper[0] == '>'))){ // Создание вершины с файлом

            if((*stack_proc)->oper[0] == '<'){
                stack_node->node->left = (tree_proc *) malloc (sizeof(tree_proc));
                now_tree = stack_node->node->left;
            }else{
                stack_node->node->right = (tree_proc *) malloc (sizeof(tree_proc));
                now_tree = stack_node->node->right;
            }
            
            *now_tree = tree_default;
            now_tree->oper = (char *) malloc (sizeof(char) * 3);
            now_tree->prog = (char **) malloc (sizeof(char *));
            now_tree->prog[0] = (char *) malloc (sizeof(char) * (strlen(mas_arg[0]) + 1));
            strcpy(now_tree->oper, (*stack_proc)->oper);
            strcpy(now_tree->prog[0], mas_arg[0]);
            pop_oper(stack_proc);

        }else{ // Сброс массива с аргументами 

            now_tree = stack_node->node;
            now_tree->prog = (char **) malloc (sizeof(char *) * (*count_arg + 1));

            for(index_for = 0; index_for < *count_arg; index_for++){
                now_tree->prog[index_for] = (char *) malloc (sizeof(char) * (strlen(mas_arg[index_for]) + 1));
                strcpy(now_tree->prog[index_for], mas_arg[index_for]);
                mas_arg[index_for][0] = '\0';
            }

            now_tree->prog[*count_arg] = NULL;

        }
        mas_arg[0][0] = '\0';
        *count_arg = 0;
    }
}

void create_tree(tree_proc **head_tree){ //Создает дерево по строке
    char str_shell[MAX_LEN_STR]; // Изначальная строка
    char mas_arg[MAX_ARG][MAX_LEN_ARG]; // Массив аргументов программы или название файла
    int len_str = 0, index_now = 0, len_arg = 0;
    int count_arg = 0, index_for = 0;
    tree_proc *new_tree;
    stack_oper *stack_proc; // Стек с операциями
    stack_tree *stack_node; // Стек с вершинами 

    printf("Please write string:\n");
    while(read(0, str_shell + len_str, 1) != -1){ // Считывание строки
        if(str_shell[len_str] == '\n')
            break;
        len_str++;
    }

    str_shell[len_str] = '\0';

    for(index_for = 0; index_for < MAX_ARG; index_for++){
        mas_arg[index_for][0] = '\0';
    }

    stack_proc = NULL;
    stack_node = (stack_tree *) malloc (sizeof(stack_tree));
    stack_node->node = *head_tree;
    stack_node->next = NULL;

    for(index_now = 0; index_now < len_str; index_now++){ // Обработка строки построчно

        if(str_shell[index_now] == '('){

            push_oper(&stack_proc, "(");
            
        }else if((str_shell[index_now] == '|') && (str_shell[index_now + 1] != '|')){ // Конвейер

            mas_arg[count_arg][len_arg] = '\0';
            if(mas_arg[count_arg][0] != '\0'){
                count_arg++;
            }
            len_arg = 0;
            mas_arg_processing(&stack_proc, stack_node, mas_arg, &count_arg);

            new_tree = (tree_proc *) malloc (sizeof(tree_proc));
            *new_tree = tree_default;
            new_tree->oper = (char *) malloc (sizeof(char) * 2);
            strcpy(new_tree->oper, "|");
            new_tree->right = (tree_proc *) malloc (sizeof(tree_proc));
            *new_tree->right = tree_default;
            new_tree->left = stack_node->node;

            if(stack_proc != NULL){

                if(!strcmp(stack_proc->oper, "(")){

                    if(stack_node->next != NULL){

                        if((stack_node->next->node->left == stack_node->node)){
                            stack_node->next->node->left = new_tree;
                        }else{
                            stack_node->next->node->right = new_tree;
                        }

                    }

                }else if(!strcmp(stack_proc->oper, "|")){

                        pop_tree(&stack_node);
                        new_tree->left = stack_node->node;

                        if(stack_node->next != NULL){

                            if((stack_node->next->node->left == stack_node->node)){
                                stack_node->next->node->left = new_tree;
                            }else{
                                stack_node->next->node->right = new_tree;
                            }

                        }

                        pop_oper(&stack_proc);

                }else{

                    stack_node->next->node->right = new_tree;

                }
            }

            pop_tree(&stack_node);
            push_tree(&stack_node, new_tree);
            push_tree(&stack_node, new_tree->right);

            push_oper(&stack_proc, "|");

        }else if((str_shell[index_now] == ';') ||
                 ((str_shell[index_now] == '&') && (str_shell[index_now + 1] == '&')) || 
                 ((str_shell[index_now] == '|') && (str_shell[index_now + 1] == '|'))){ // Последовательная операция

            mas_arg[count_arg][len_arg] = '\0';
            if(mas_arg[count_arg][0] != '\0'){
                count_arg++;
            }
            len_arg = 0;
            mas_arg_processing(&stack_proc, stack_node, mas_arg, &count_arg);

            new_tree = (tree_proc *) malloc (sizeof(tree_proc));
            *new_tree = tree_default;
            new_tree->oper = (char *) malloc (sizeof(char) * 3);

            switch(str_shell[index_now]){

            case ';': strcpy(new_tree->oper, ";"); break;
            case '|': strcpy(new_tree->oper, "||"); break;
            case '&': strcpy(new_tree->oper, "&&");  break;
            default:

            }

            new_tree->right = (tree_proc *) malloc (sizeof(tree_proc));
            *new_tree->right = tree_default;
            new_tree->left = stack_node->node;

            if(stack_proc != NULL){
                if(!strcmp(stack_proc->oper, "(")){

                    if(stack_node->next != NULL){

                        if(stack_node->next->node->left == stack_node->node){
                            stack_node->next->node->left = new_tree;
                        }else{
                            stack_node->next->node->right = new_tree;
                        }

                    }

                }else{
                    
                    if((stack_proc->next != NULL) && strcmp(stack_proc->next->oper, "(")){
                        pop_oper(&stack_proc);
                        pop_tree(&stack_node);
                    }

                    if(stack_node->next->next != NULL){

                        if(stack_node->next->next->node->left == stack_node->next->node){
                            stack_node->next->next->node->left = new_tree;
                        }else{
                            stack_node->next->next->node->right = new_tree;
                        }

                    }

                    new_tree->left = stack_node->next->node;

                }
            }

            if((stack_proc != NULL) && (strcmp(stack_proc->oper, "("))){
                pop_oper(&stack_proc);
                pop_tree(&stack_node);
            }

            pop_tree(&stack_node);
            push_tree(&stack_node, new_tree);
            push_tree(&stack_node, new_tree->right);

            switch(str_shell[index_now]){

            case ';': push_oper(&stack_proc, ";"); break;
            case '|': push_oper(&stack_proc, "||"); index_now++; break;
            case '&': push_oper(&stack_proc, "&&"); index_now++; break;
            default:

            }

        }else if((str_shell[index_now] == ')')){

            mas_arg[count_arg][len_arg] = '\0';
            if(mas_arg[count_arg][0] != '\0'){
                count_arg++;
            }
            len_arg = 0;
            mas_arg_processing(&stack_proc, stack_node, mas_arg, &count_arg);

            while((stack_proc != NULL) && strcmp(stack_proc->oper, "(")){
                pop_oper(&stack_proc);
                pop_tree(&stack_node);
            }

            pop_oper(&stack_proc);

        }else if((str_shell[index_now] == '<') || (str_shell[index_now] == '>')){

            mas_arg[count_arg][len_arg] = '\0';
            if(mas_arg[count_arg][0] != '\0'){
                count_arg++;
            }
            len_arg = 0;
            mas_arg_processing(&stack_proc, stack_node, mas_arg, &count_arg);

            if(str_shell[index_now] == '<'){
                push_oper(&stack_proc, "<");
            }else if(str_shell[index_now] == str_shell[index_now + 1]){
                index_now++;
                push_oper(&stack_proc, ">>"); 
            }else{
                push_oper(&stack_proc, ">");
            }

        }else if((str_shell[index_now] == '&')){
            
            background();
            break;

        }else if((str_shell[index_now] == ' ')){

            if(mas_arg[0][0] != '\0'){
                mas_arg[count_arg][len_arg] = '\0';
                count_arg++;
                len_arg = 0;
            }
            
        }else{

            mas_arg[count_arg][len_arg] = str_shell[index_now];
            len_arg++;

        }
    }

    mas_arg[count_arg][len_arg] = '\0';
    if(mas_arg[count_arg][0] != '\0'){
        count_arg++;
    }
    len_arg = 0;
    mas_arg_processing(&stack_proc, stack_node, mas_arg, &count_arg);

    while(stack_node->next != NULL){
        pop_tree(&stack_node);
        mas_arg[count_arg][len_arg] = '\0';
    }

    *head_tree = stack_node->node;
    pop_tree(&stack_node);

    while(stack_proc != NULL){
        pop_oper(&stack_proc);
    }
}

int main(){
    int pid, file_desc = 0;
    tree_proc *head_tree; // Вершина дерева

    head_tree = (tree_proc *) malloc (sizeof(tree_proc));
    *head_tree = tree_default;

    create_tree(&head_tree);

    if((pid = fork()) == 0){
        if(head_tree->oper != NULL){
            choose_func_oper(head_tree);
        }else{

            if(head_tree->left != NULL){
                file_desc = choose_func_oper(head_tree->left);
                dup2(file_desc, 0);
                close(file_desc);
            }

            if(head_tree->right != NULL){
                file_desc = choose_func_oper(head_tree->right);
                dup2(file_desc, 1);
                close(file_desc);
            }

            execvp(head_tree->prog[0], head_tree->prog);
        }
    }else{
        if(pid == -1){
            printf("Error: can't create process\n");
            exit(1);
        }
    }
    wait(NULL);
    return 0;
}