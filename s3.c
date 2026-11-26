#include "s3.h"

///Simple for now, but will be expanded in a following section
void construct_shell_prompt(char shell_prompt[])
{
    ///string copy to array shell_prompt
    strcpy(shell_prompt, "[s3]$ ");
}

///Prints a shell prompt and reads input from the user
void read_command_line(char line[])
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