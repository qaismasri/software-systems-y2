# s3: software systems shell

This is our shell for the ELEC50014 Software Systems coursework. It's a small version of Bash written in C: you type a command, it runs it, and it gives you the prompt back. Under the hood it's the fork → exec → wait pattern from Lectures 2 and 3.

So far we've finished **Task 1 (basic commands)**. This README will grow as we add redirection, `cd`, pipes and the rest.

## Building and running

The shell uses Unix system calls (`fork`, `execvp`, `wait`), so it has to be built on Linux. We use WSL on Windows. It won't compile with a normal Windows compiler.

```
gcc -Wall *.c -o s3
./s3
```

You'll get a prompt that looks like this:

```
[s3]$
```

Type a command the same way you would in Bash. Type `exit` to leave.

## What it does at the moment

- Runs any program that's installed on the system, with its arguments: `ls -l`, `whoami`, `pwd`, `cal -y`, `grep -n word file`, `wc file`, `cat`, `less`, `man cat` and so on.
- If you just press Enter, it shows a new prompt and doesn't crash.
- If the command doesn't exist (e.g. `blahblah`), it prints an error and carries on.
- `exit` quits the shell.

We didn't write any code for `ls`, `grep` etc. They're ordinary programs that live in folders like `/usr/bin` (`which ls` shows where). Our shell only has to find and start them. Anything installed on the machine works without any changes to our code.

## How the code is laid out

| File | What's in it |
|---|---|
| `s3main.c` | `main`, the loop that keeps the shell running |
| `s3.h` | the header: library includes, constants, and declarations of every function |
| `s3.c` | the actual function bodies |

`s3.h` is basically a list of promises. It says which functions exist and what they take, and `s3.c` has to keep those promises by defining them. Each `.c` file does `#include "s3.h"` so the compiler knows about all the functions before they're used.

The constants live in `s3.h` too:

```c
#define MAX_LINE 1024        // longest command line we accept
#define MAX_ARGS 128         // most words in one command
#define MAX_PROMPT_LEN 256
```

C arrays need a fixed size when the program is compiled, so these set the limits. Putting them in one place means we only have to change a number once.

## Walking through one command

Here's what happens when you type `ls -l`.

### 1. The main loop (`s3main.c`)

```c
while (1) {
    read_command_line(line);
    parse_command(line, args, &argsc);
    launch_program(args, argsc);
    reap();
}
```

`while (1)` loops forever on purpose, because a shell should keep going until the user says stop. The only way out is the `exit` command, which ends the process from inside `launch_program`.

### 2. Reading the line (`read_command_line`)

This prints the prompt, then uses `fgets` to read what the user typed into `line`. `fgets` keeps the newline from pressing Enter, so we overwrite it with `'\0'`:

```c
line[strlen(line) - 1] = '\0';
```

Without that, `execvp` would go looking for a program called `"ls\n"` and wouldn't find it.

### 3. Splitting it into words (`parse_command`)

`strtok` splits the line on spaces. It doesn't copy anything. It swaps each space for a `'\0'`, so one string turns into several, and `args` just points into them:

```
line:  l s \0 - l \0
       ↑      ↑
    args[0] args[1]     args[2] = NULL, argsc = 2
```

The list has to end with `NULL`, because that's how `execvp` knows where the arguments stop. That's also why the loop only goes up to `MAX_ARGS - 1`: we always keep one slot free for the `NULL`.

`argsc` is passed in as `&argsc` (its address), so `parse_command` can change the count that `main` sees by writing `*argsc = ...`. `args` doesn't need the `&`, because arrays are passed as an address automatically.

### 4. Deciding what to do (`launch_program`)

```c
void launch_program(char *args[], int argsc)
{
    // empty line: nothing to run
    if (argsc == 0) {
        return;
    }

    // built-in: exit
    if (!strcmp(args[ARG_PROGNAME], "exit")) {
        exit(0);
    }

    // anything else: start a child to run it
    pid_t rc = fork();

    if (rc < 0) {
        perror("forking child failed");
        exit(1);
    }
    else if (rc == 0) {
        child(args, argsc);
    }
}
```

The order of the checks matters:

- **Empty line first.** On an empty line `args[0]` is `NULL`, and calling `strcmp` on `NULL` crashes. So we leave before we ever look at it.
- **`exit` before `fork`.** `exit` has to end the *shell*. If we forked first, only the child would exit and the shell would carry on. That's why `exit` is a built-in: it changes the shell process itself, which a child can't do.
- **`strcmp` returns 0 when the strings match**, which is the opposite of what you might guess, so we use `!strcmp(...)`.

`fork()` returns `-1` if it failed, `0` inside the new child, and the child's process ID inside the parent. `pid_t` is just the type for a process ID (an integer on Linux).

`ARG_PROGNAME` comes from an enum in `s3.h`. It's just a readable name for `0`, the slot that holds the program name.

### 5. Turning the child into the program (`child`)

```c
void child(char *args[], int argsc)
{
    execvp(args[ARG_PROGNAME], args);

    // only get here if execvp failed
    perror("failed to execute child process");
    exit(1);
}
```

`execvp` replaces the child's code with the program named in `args[0]` and hands it the whole `args` list, so `ls` gets its `-l`. The `p` means it searches the folders in `PATH`, so the user can type `ls` instead of `/usr/bin/ls`.

If `execvp` works, it never returns, so the lines after it only run when it fails. Then we print why and call `exit(1)`. That exit matters: without it, the failed child would return into `main`'s loop and you'd have two shells running and need to type `exit` twice.

### 6. Cleaning up (`reap`)

Back in the parent, `main` calls `reap()`, a one-line wrapper around `wait(NULL)` defined in `s3.h`. The shell waits for the child to finish, so the next prompt doesn't appear in the middle of `ls`'s output. This also cleans up the finished child so it doesn't stay around as a zombie.

## A few decisions we made

**`perror` for errors, not `printf`.** `printf` writes to standard output (file descriptor 1). `perror` writes to standard error (file descriptor 2), and it adds the reason the system call failed, e.g. `No such file or directory`. This will matter for Task 2. If someone runs `ls missing > out.txt`, only fd 1 goes into the file, so the error still shows up on screen instead of being hidden in `out.txt`.

**Waiting stays in `main`.** We thought about using `waitpid` with a status variable inside `launch_program`, so the shell could remember whether the last command succeeded (like `$?` in Bash). It's written but commented out at the bottom of `launch_program`, because nothing uses the status yet and `main` already reaps the child. We'll probably bring it back for job control or `&&`.

**Fork failure exits the shell.** At the moment, if `fork` fails we print the error and quit. A real shell would probably just show the prompt again, so we might change this to a `return`.

## Testing

We tested in WSL by running `./s3` and typing commands by hand:

| Input | What happened |
|---|---|
| just Enter | new prompt |
| `whoami` | prints our username |
| `pwd` | prints the current folder |
| `ls -l`, `ls -R` | listing as normal |
| `wc file`, `grep -n word file` | same output as in Bash |
| `cat` (then Ctrl+D) | echoes what we type |
| `less s3.c`, `man cat` | open properly, `q` quits |
| `blahblah` | `failed to execute child process: No such file or directory`, shell keeps running |
| `exit` | quits first time |

(`cal` needs `sudo apt install ncal` on a fresh WSL install.)

## Known limitations (for now)

- `cd` doesn't work yet. There's no `cd` program to exec, so it has to be a built-in in the shell (Task 3).
- `>`, `<`, `|` and `;` are passed to the program as ordinary words, so redirection, pipes and batches don't work yet (Tasks 2, 4, 5).
- Words are only split on spaces, not tabs.
- Quotes aren't handled, so `grep "two words" file` gets split into separate arguments.

## What's next

- Task 2: redirection with `open` and `dup2`
- Task 3: `cd` (including `cd` and `cd -`) and showing the current directory in the prompt
- Tasks 4 to 7: pipes, `;`, and subshells