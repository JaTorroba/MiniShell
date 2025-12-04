#!/bin/bash
mkdir -p build
mkdir -p bin
gcc -Wall -Wextra -c src/myshell.c -Iinclude -o build/myshell.o
if [ $? -ne 0 ]; then
    echo "Error while generating build";
    exit 1
fi
gcc build/myshell.o -o bin/myshell -Llib -lparser -static
if [ $? -ne 0 ]; then
    echo "Error linking build to lib";
    exit 1
fi
echo "Executable 'myshell' created in bin/myshell";
#Debugging:
#gcc -g on generating executable command
#gdb executable
#l [n] | shows next n lines of code
#b n | creates breakpoint in line n
#r | run
#n | executes next line