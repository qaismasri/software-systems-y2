#include "s3.h"

///Simple for now, but will be expanded in a following section
void construct_shell_prompt(char shell_prompt[]){
    char cwd[MAX_PROMPT_LEN-6]; // current working directory

    if (getcwd(cwd, MAX_PROMPT_LEN-6) == NULL) {
        /// if folder not found, use default
        strcpy(shell_prompt, "[s3]$");
        return;
    }

    snprintf(shell_prompt, MAX_PROMPT_LEN, "[%s s3]$", cwd);
}

///Prints a shell prompt and reads input from the user
void read_command_line(char line[], char lwd[])
{
    char shell_prompt[MAX_PROMPT_LEN];
    construct_shell_prompt(shell_prompt);
    printf("%s", shell_prompt);

    ///See man page of fgets(...)
    ///syntax of fgets here: https://en.cppreference.com/c/io/fgets
    if (fgets(line, MAX_LINE, stdin) == NULL)
    {
        perror("fgets failed");
        exit(1);
    }   
    ///Remove newline (enter)
    ///replaces [\n with \0 and reduces strlen by 1]
    line[strlen(line) - 1] = '\0';
}

void parse_command(char line[], char *args[], int *argsc)
{
    ///Implements simple tokenization (space delimited)
    ///Note: strtok puts '\0' (null) characters within the existing storage, 
    ///to split it into logical cstrings.
    ///There is no dynamic allocation.

    ///See the man page of strtok(...)
    ///syntax of strtok here: https://www.w3schools.com/c/ref_string_strtok.php
    char *token = strtok(line, " ");
    *argsc = 0;
    
    /// MAX_ARGS - 1 keeps 1 free slot for the NULL
    while (token != NULL && *argsc < MAX_ARGS - 1)
    {
        args[(*argsc)++] = token;
        token = strtok(NULL, " ");
    }
    
    args[*argsc] = NULL; ///args must be null terminated
}

///Launch related functions
void child(char *args[], int argsc)
{
    execvp(args[ARG_PROGNAME], args);

    ///error if execvp doesnt execute, with exit out of shell
    perror("failed to execute child process");
    exit(1);
}

void launch_program(char *args[], int argsc)
{
    ///Implement this function:

    ///fork() a child process.
    ///In the child part of the code,
    ///call child(args, argv)
    ///For reference, see the code in lecture 2.

    ///Handle the 'exit' command;
    ///so that the shell, not the child process,
    ///exits.

    ///CASE 1: No command entered, empty line
    if(argsc == 0){
        return;
        ///void type used, not int
    }

    ///CASE 2: user initiates exit
    if(!strcmp(args[ARG_PROGNAME], "exit")){
        exit(0);
    }

    ///CASE 3: user performs action
    pid_t rc = fork();

    if(rc < 0){
        perror("forking child failed");
        exit(1);
    }
    else if(rc == 0){
        child(args, argsc);
    }

    /* Extension: DEBUGGING WHETHER CHILD EXITED CORRECTLY:
    FETCH STATUS OF WAIT BEFORE REAPING
    
    else{
        int status;
        waitpid(rc, &status, 0);

        if (WIFEXITED(status)){
            last_exit_status = WEXITSTATUS(status);
        }
        else{
            last_exit_status = -1;
        }
    }
    */

}

int command_with_redirection(char line[]){
    if(strchr(line, '>') || strchr(line, '<')){
        return 1;
    }
    else{
        return 0;
    }
}

int find_redirector_index(char *args[], int argsc){
    for(int i = 0; i < argsc; i++){
        if(strcmp(args[i], "<") == 0 || strcmp(args[i], ">") == 0 || strcmp(args[i], ">>") == 0){
            return i;
        }
    }
    return -1; // no value found
}

void child_with_output_redirected(char *args[], int argsc, char *filename, int is_append){
    int flags = O_WRONLY | O_CREAT;
    if(is_append){
        flags = flags | O_APPEND;
    }
    else{
        flags = flags | O_TRUNC; // > operator
    }
    
    int fd = open(filename, flags, 0644);
    if(fd == -1){
        perror("error opening file");
        exit(1);
    }
    int check = dup2(fd, STDOUT_FILENO); //STDOUT_FILENO is terminal output
    if(check == -1){
        perror("error reading file input");
        exit(1);
    }
    close(fd);
    child(args, argsc);
}

void child_with_input_redirected(char *args[], int argsc, char *filename){
    int flags = O_RDONLY;
    int fd = open(filename, flags);
    if(fd == -1){
        perror("error opening file");
        exit(1);
    }
    int check = dup2(fd, STDIN_FILENO); // STDIN is keyboard / peripheral input
    if(check == -1){
        perror("error reading file input");
        exit(1);
    }
    close(fd);
    child(args, argsc);
}

void launch_program_with_redirection(char *args[], int argsc){
    int pos = find_redirector_index(args, argsc);

    ///ERROR1: pos not found
    if(pos == -1){
        perror("syntax error with operator");
        return;
    }

    ///ERROR2: Filename not found
    if(pos + 1 >= argsc){
        perror("no filename supplied");
        return;
    }

    char *operator = args[pos];
    char *filename = args[pos + 1];

    int op_out = !strcmp(args[pos], ">>");

    args[pos] = NULL;
    argsc = pos;

    pid_t rc = fork();

    if (rc < 0) {
        perror("error forking child");
        return;
    }

    else if (rc == 0) {
        if (strcmp(operator, "<") == 0)
            child_with_input_redirected(args, argsc, filename);
        else
            child_with_output_redirected(args, argsc, filename, op_out);
    }
}

int is_cd(char line[]){
    ///line starts with either "cd *" or is just "cd"
    if(strncmp(line, "cd", 2) == 0 && (line[2] == ' ' || line[2] == '\0')){
        return 1;
   }
   else return 0;
}

void init_lwd(char lwd[]){
    if(getcwd(lwd, MAX_PROMPT_LEN-6) == NULL){
        perror("error fetching directory");
    }
}

void run_cd(char *args[], int argsc, char lwd[]){

    /// separate cases of cd input
    char *dest;

    ///CASE 1: cd
    if(argsc == 1){
        dest = getenv("HOME");
        if(dest == NULL){
            perror("Home cannot be found");
            return;
        }
    }

    ///CASE 2: cd aaa bbb (error)
    else if(argsc > 2){
        perror("too many arguments for cd");
        return;
    }
    else{
        ///CASE 3: cd -
        if(strcmp(args[1], "-") == 0){
            dest = lwd;
        }
        ///CASE 4: cd aaa
        else{
            dest = args[1];
        }
    }

    char cwd[MAX_PROMPT_LEN-6];
    if (getcwd(cwd, MAX_PROMPT_LEN-6) == NULL) {
        perror("current directory not found");
        return;
    }

    if (chdir(dest) == -1) {
        perror("requested directory not found");
        return;
    }

    strcpy(lwd, cwd);
}
