rm -f bcc.o bin/bcc a.out
cd ../bci; sh compile.sh; cd ../bcc
gcc -c -I../bci/inc -std=c11 -o bcc.o bcc.c

ar rc lib/libbcc.a bcc.o
cp bcc.h inc

gcc -Iinc -Llib -I../bci/inc -L../bci/lib -std=c11 -o bin/bcc main.c -lbcc -lbci
