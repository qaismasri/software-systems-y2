A .h file is a header file. It's a list of things your other files need to know about, without the actual code. Think of it as a table of contents, or the menu from earlier: it says what exists, and the .c files say how it works.

s3main.c takes a Command Line input, separates each word / command into an array of args (parse), and executes each arg individually.

s3.h has a header file that defines which functions and standard libraries will be used.
Every function declared in s3.h is like a promise, and the declared function must be defined in s3.c (keeping that promise).

Use perror vs printf to print errors since perror prints level 2 descriptors (printf prints level 1 descriptors)
if printf is used, if output is written to external fstreams like out.txt, error messages written to that file instead of terminal.

Build shell with gcc -Wall *.c -o s3
./s3

TASK2:

If input output fstreams are involved using different files, redirection is required.

Need to cut args before <, > or >> operators as commands do not understand these operators (eg. ls)

fd	name	used for
0	stdin	input (the keyboard)
1	stdout	normal output (the screen)
2	stderr	error messages

open or dup2 executed from child process to prevent parent pointing at out.txt

Flags include O_WRONLY, O_CREAT, O_RDONLY, O_APPEND included in s3.h

open() takes 0644 for output redirected as permission to create a new file.

TASK 3: cd implementation

getcwd returns path of current working directory
lwd is last working directory

could think abt make it nicer with space, max prompt len - 7 then