#!/bin/sh

make clean
./configure --with-libnotify
pvs-studio-analyzer trace -- make
pvs-studio-analyzer analyze -a "GA;64;OP;CS;OWASP" --security-related-issues --intermodular
plog-converter -o tmp -a "GA:1,2;64:1;OP:1,2,3;CS:1;OWASP:1" -t fullhtml PVS-Studio.log
