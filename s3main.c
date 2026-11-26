#include "s3.h"

int main(int argc, char *argv[]){

    ///Stores the command line input
    ///params have been defined in s3.h
    char line[MAX_LINE];

    ///Stores pointers to command arguments.
    ///The first element of the array is the command name.
    ///empty array created here
    char *args[MAX_ARGS];

    ///Stores the number of arguments
    int argsc;

    while (1) {
        read_command_line(line);

        parse_command(line, args, &argsc);

        launch_program(args, argsc); 

        reap();
    }

    return 0;
    /// 0 means successful return
    /// only time return used is when user exits program: TO ADD
    /// but until then shell runs infinite while loop (1)
    
}
