#!/bin/bash

usage(){
    echo "$0 filename"
}

if [ "$#" -ne 1 ]; then
    usage
    exit 1
fi

name=$1
if [ ! -e "${name}" ]; then
    echo "${name} don't exist."
    exit 2
fi
filename=$(echo ${name} | awk -F. '{print $1}')
#nasm -f elf -F stabs ${name}  -l ${filename}.list
#ld -m elf_i386  -o example  ${filename}.o
nasm -f elf64 -F stabs ${name}  -l ${filename}.list
ld -m elf_x86_64  -o example  ${filename}.o

