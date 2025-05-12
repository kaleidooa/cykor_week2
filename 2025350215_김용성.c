#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <signal.h>

#define MAX_SIZE 100

typedef struct Directory{
    char name[MAX_SIZE];
    struct Directory *parent;
    struct Directory *subdirectories[10];
    int subdirectory_count;
} Directory;

Directory* create_directory(char *name, Directory*parent){
    Directory *new_dir = (Directory*)malloc(sizeof(Directory));
    strcpy(new_dir->name,name);
    new_dir->parent = parent;  
    new_dir->subdirectory_count = 0;
    if(parent != NULL){
        parent-> subdirectories[parent->subdirectory_count] = new_dir;
        parent-> subdirectory_count ++;
    }
    return new_dir;
}

int cd(Directory **, char *, Directory *, Directory *);
int ls(Directory *);
int pwd(Directory *, int);


void split_sequence(char *input, char **seq) {
    char *tok = strtok(input, ";");
    int i = 0;
    while (tok) {
        while (*tok==' '){
            tok++;
        } 
        char *end = tok + strlen(tok)-1;
        while (end>tok && *end==' '){
            *end--='\0';
        } 
        seq[i++] = tok;
        tok = strtok(NULL, ";");
    }
    seq[i] = NULL;
}

void split_pipeline(char *input, char **cmds) {
    char *tok = strtok(input, "|");
    int i = 0;
    while (tok) {
        while (*tok == ' ') tok++;
        char *end = tok + strlen(tok) - 1;
        while (end > tok && *end == ' ') *end-- = '\0';

        cmds[i++] = tok;
        tok = strtok(NULL, "|");
    }
    cmds[i] = NULL;
}

int execute_command(char* input , Directory **current_dir , Directory *root , Directory *home){
    char argument[MAX_SIZE] = {0,};
    char command[MAX_SIZE] = {0,};

    memset(argument , 0 , MAX_SIZE);
    sscanf(input, "%s %s" , command , argument);

    if(strcmp(command , "exit") == 0){
        exit(0);
    }
    else if(strcmp(command, "cd") == 0){
        return cd(current_dir,argument,root,home);
    }
    else if(strcmp(command, "ls") ==0){
        ls(*current_dir);
        return 0;
    }
    else if(strcmp(command , "pwd") == 0){
        pwd(*current_dir , 1);
        printf("\n");
        return 0;
    }
    else{
        printf("%s: command not found\n" , command); 
        return 1;
    }

    return 0;
}

typedef enum { OP_NONE, OP_AND, OP_OR } Op;
typedef struct { char *cmd; Op op; } Logical;

int split_logical(char *line, Logical *chain) {
    int idx = 0;
    char *p = line;
    while (p && *p) {
        chain[idx].cmd = p;
        chain[idx].op = OP_NONE;
        char *a = strstr(p, "&&");
        char *o = strstr(p, "||");
        char *sep = NULL;
        if (a && (!o || a < o)){ 
            sep = a; 
            chain[idx].op = OP_AND; 
        }
        else if (o){ 
            sep = o; 
            chain[idx].op = OP_OR;  
        }
        if (!sep) { 
            idx++; 
            break; 
        }
        *sep = '\0';
        idx++;
        p = sep + 2;
    }
    chain[idx].cmd = NULL;
    return idx+1;
}

int execute_pipe(char *input, Directory **current_dir, Directory *root, Directory *home) {
    char *cmds[10];
    split_pipeline(input, cmds);

    int num_cmds = 0;
    while (cmds[num_cmds] != NULL) {
        num_cmds++;
    }

    int fd[10][2];
    int in_fd = STDIN_FILENO;  

    for (int i = 0; i < num_cmds; i++) {
        if (i < num_cmds - 1) {
            if (pipe(fd[i]) == -1) {
                perror("pipe");
                exit(EXIT_FAILURE);
            } 
        }

        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        } 
        else if (pid == 0) {

        
            if (in_fd != STDIN_FILENO) {
                dup2(in_fd, STDIN_FILENO);
                close(in_fd);
            }

            if (i < num_cmds - 1) {
                dup2(fd[i][1], STDOUT_FILENO);
                close(fd[i][0]);
                close(fd[i][1]);
            }

            for (int j = 0; j < i; j++) {
                close(fd[j][0]);
                close(fd[j][1]);
            }

            char *argv[MAX_SIZE / 2];
            int argc = 0;
            char *token = strtok(cmds[i], " ");
            while (token) {
                argv[argc++] = token;
                token = strtok(NULL, " ");
            }
            argv[argc] = NULL;

            if (!strcmp(argv[0],"cd")||!strcmp(argv[0],"ls")||!strcmp(argv[0],"pwd")) {
                execute_command(cmds[i], current_dir, root, home);
                exit(0);
            }

            execvp(argv[0], argv);
            perror("execvp");
            exit(EXIT_FAILURE);
        } 
        else {

            if (in_fd != STDIN_FILENO) {
                close(in_fd);
            }

            if (i < num_cmds - 1) {
                close(fd[i][1]);
                in_fd = fd[i][0];
            }
        }
    }

    int status = 0;
    for(int i = 0 ; i < num_cmds; i++){
        int w;
        wait(&w);
        status = w;
    }

    if(WIFEXITED(status)){
        return WEXITSTATUS(status);
    }   
    return 1;
}

void free_memory(Directory *dir){
    for(int i = 0; i<dir->subdirectory_count; i++){
        free_memory(dir->subdirectories[i]);
    }
    free(dir);
}

int cd(Directory **current_dir, char *where, Directory *root , Directory *home){
    
    char *token;
    Directory *target_dir;

    if(strlen(where) == 0){
        *current_dir = home;
        return 0;
    }

    if(where[0] == '/'){
        target_dir = root;
        where++;
    }
    else{
        target_dir = *current_dir;
    }

    token = strtok(where , "/");

    while(token != NULL){

        if(strcmp(token, "..") ==0){
            if(target_dir->parent != NULL){
                target_dir = target_dir->parent;
            }
        }
        else{
            int isfound = 0;
            for(int i = 0 ;  i < target_dir->subdirectory_count; i++){
                if(strcmp(token, target_dir->subdirectories[i]->name) == 0){
                    target_dir = target_dir->subdirectories[i];
                    isfound = 1;
                    break;
                }   
            }    
            if(isfound == 0){
                printf("-bash : cd : %s: No such file or directory\n" , where);
                return 1;
            }
        }

        token = strtok(NULL , "/");
    }

    *current_dir = target_dir;
    return 0;
}

int ls(Directory *current_dir){
    int count = 0;

    for(int i = 0 ; i < current_dir->subdirectory_count; i++){
        printf("%s   " , current_dir -> subdirectories[i] -> name);
        count++;
        if(count == 4){
            printf("\n");
            count = 0;
        }
    }
    if(current_dir->subdirectory_count == 0){
        printf("\n");
        return 0;
    }
    printf("\n");

    return 0;
}

int pwd(Directory *current_dir , int isHead){
    if(current_dir == NULL){
        return 0;
    }

    if(current_dir->parent!=NULL){
        pwd(current_dir->parent , 0);
    }

    if(strcmp(current_dir->name , "root") == 0){
        printf("/");
    }
    else{
        printf("%s" , current_dir->name);
        if(!isHead){
            printf("/");
        }
    }

    return 0;
}

int run_logical_chain(char *line, Directory **cdp, Directory *root, Directory *home) {
    Logical chain[10];
    int n = split_logical(line, chain);
    int prev=0;
    for(int i=0; i<n && chain[i].cmd; i++){
        if (i > 0) {
            if (chain[i-1].op == OP_AND && prev != 0){
                continue;
            }
            if (chain[i-1].op == OP_OR  && prev == 0){
                continue;
            } 
        }
        if (strchr(chain[i].cmd, '|')){
            prev = execute_pipe(chain[i].cmd, cdp, root, home);
        }   
        else{
            prev = execute_command(chain[i].cmd, cdp, root, home);
        }
    }

    return prev;
}

int main(){

    char username[MAX_SIZE] = "kaleido";
    char hostname[MAX_SIZE] = "iamthehost";
    char input[MAX_SIZE] = {0,};

    char command[MAX_SIZE];
    char argument[MAX_SIZE];

    signal(SIGCHLD, SIG_IGN);

    Directory *root = create_directory("root" , NULL);
    Directory *home = create_directory("home" , root);
    Directory *etc = create_directory("etc" , root);
    Directory *bin = create_directory("bin" , root);
    Directory *kaleido = create_directory("kaleido" , home);
    Directory *user = create_directory("user" , home);
    Directory *current_dir = root;

    while(1){

        if(current_dir == kaleido){
            printf("[%s@%s:~]$" , username,hostname);
        }
        else{
            printf("[%s@%s:" , username, hostname);
            pwd(current_dir,1);
            printf("]$");
        }
        fflush(stdout);

        fgets(input , 100 , stdin);      
        input[strcspn(input, "\n")] = '\0';

        int bg = 0;
        size_t len = strlen(input);
        if(len > 0 && input[len-1] == '&'){
            bg = 1;
            input[--len] = '\0';
            while(len > 0 && input[len -1] == '&'){
                input[--len] = '\0';
            }
        }
        if (bg) {
            pid_t pid = fork();
            if (pid < 0) {
                perror("fork");
            } else if (pid == 0) {
                run_logical_chain(input, &current_dir, root, home);
                exit(0);
            }
            continue;
        }

        if(strchr(input, ';')){
            char *seq[10] = {0,};   
            split_sequence(input, seq);
            for(int i = 0 ; seq[i]; i++){
                run_logical_chain(seq[i] , &current_dir, root, home);
            }
        }   
        else{
            run_logical_chain(input, &current_dir, root, home);
        }
    }

    free_memory(root);

    return 0;
}
