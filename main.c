#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

void cd(Directory **, char *, Directory *, Directory *);
void ls(Directory *);
void pwd(Directory *, int);


void split_pipeline(char* input, char** commands){
    char* token = strtok(input, "|");
    int i = 0;

    while(token != NULL){
        commands[i] = token;
        i++;
        token = strtok(NULL , "|");
    }

    commands[i] = NULL;

}

void execute_command(char* input , Directory **current_dir , Directory *root , Directory *home){
    
    char argument[MAX_SIZE] = {0,};
    char command[MAX_SIZE] = {0,};

    memset(argument , 0 , MAX_SIZE);
    sscanf(input, "%s %s" , command , argument);

    if(strcmp(command , "exit") == 0){
        exit(0);
    }
    else if(strcmp(command, "cd") == 0){
        cd(current_dir,argument,root,home);
    }
    else if(strcmp(command, "ls") ==0){
        ls(*current_dir);
    }
    else if(strcmp(command , "pwd") == 0){
        pwd(*current_dir , 1);
        printf("\n");
    }
    else{
        printf("%s: command not found\n" , command); 
    }
    

}


void free_memory(Directory *dir){
    for(int i = 0; i<dir->subdirectory_count; i++){
        free_memory(dir->subdirectories[i]);
    }
    free(dir);
}

void cd(Directory **current_dir, char *where, Directory *root , Directory *home){
    
    char *token;
    Directory *target_dir;

    if(strlen(where) == 0){
        *current_dir = home;
        return;
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
                return;
            }
        }

        token = strtok(NULL , "/");
    }

    *current_dir = target_dir;
}

void ls(Directory *current_dir){
    int count = 0;

    for(int i = 0 ; i < current_dir->subdirectory_count; i++){
        printf("%s   " , current_dir -> subdirectories[i] -> name);
        if(count == 4){
            printf("\n");
            count = 0;
        }
    }
    if(current_dir->subdirectory_count == 0){
        return;
    }
    printf("\n");
}

void pwd(Directory *current_dir , int isHead){
    if(current_dir == NULL){
        return;
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
}

int main(){

    char username[MAX_SIZE] = "kaleido";
    char hostname[MAX_SIZE] = "iamthehost";
    char input[MAX_SIZE] = {0,};

    char command[MAX_SIZE];
    char argument[MAX_SIZE];

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

        fgets(input , 100 , stdin);      
        input[strcspn(input, "\n")] = '\0';
        
        if(strchr(input , '|')){
            char *cmds[10] = {0,};
            split_pipeline(input , cmds);

            for(int i = 0; cmds[i]; i++){
                execute_command(cmds[i] , &current_dir , root, kaleido);    
            }

            continue;
        }

        execute_command(input, &current_dir,root,kaleido);
    }

    free_memory(root);

    return 0;
}