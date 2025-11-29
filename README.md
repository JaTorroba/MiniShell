## 💻 MiniShell 
MiniShell is an academic proyect for **Operating Systems** subject which focuses on interpreting and executing commands using a structured approach.

## Key Implementation Components:

The program's core logic involves a loop that repeatedly:

**Displays a prompt (msh> ).**

**Reads an input line.**

**Parses the line using the provided parser library**. This library includes the function tline* tokenize(char * str) and the data structures tline and tcommand to manage command sequences, arguments, and redirections.

**Executes all commands simultaneously by creating multiple child processes.**

**Manages inter-process communication using pipes as necessary.**

**Handles file redirections (input, output, and error).**

**Manages background execution** (using &) without blocking the shell.

**Manages job control and signal handling** for signals like SIGINT (Ctrl-C) and SIGTSTP (Ctrl-Z).

**Implements internal commands** such as cd, jobs, bg, exit, and umask.
