#!/bin/bash
mkdir -p build
mkdir -p bin
gcc -Wall -Wextra -c src/test.c -Iinclude -o build/test.o
if [ $? -ne 0 ]; then
    echo "Error al compilar en build";
    exit 1
fi
gcc build/test.o -o bin/test -Llib -lparser -static
if [ $? -ne 0 ]; then
    echo "Error al enlazar build con lib";
    exit 1
fi
echo "Ejecutable 'myshell' creado en bin/myshell"