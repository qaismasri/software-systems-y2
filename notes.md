A .h file is a header file. It's a list of things your other files need to know about, without the actual code. Think of it as a table of contents, or the menu from earlier: it says what exists, and the .c files say how it works.

s3main.c takes a Command Line input, separates each word / command into an array of args (parse), and executes each arg individually.

s3.h has a header file that defines which functions and standard libraries will be used.
Every function declared in s3.h is like a promise, and the declared function must be defined in s3.c (keeping that promise).

