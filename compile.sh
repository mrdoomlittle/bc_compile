rm -f bcc.o bin/bcc a.out *.o
cd ../bitct; sh compile.sh; cd ../bcc;
cd ../bci; sh compile.sh; cd ../bcc;
C_INC="-I../bci/inc -I../bitct/inc"
C_LIB="-L../bci/lib -L../bitct/lib"
gcc -c $C_INC -std=gnu11 -o bcc.o bcc.c
gcc -c $C_INC -std=gnu11 -o lex.o lex.c
gcc -c $C_INC -std=gnu11 -o parse.o parse.c
gcc -c $C_INC -std=gnu11 -o gen.o gen.c
gcc -c $C_INC -std=gnu11 -o debug.o debug.c
gcc -c $C_INC -std=gnu11 -o vec.o vec.c
gcc -c $C_INC -std=gnu11 -o buff.o buff.c
gcc -c $C_INC -std=gnu11 -o map.o map.c
ar rc lib/libmdl-bcc.a bcc.o lex.o parse.o gen.o debug.o vec.o buff.o map.o
cp bcc.h inc/mdl

gcc -Iinc -Llib $C_INC $C_LIB -std=gnu11 -o bin/bcc main.c -lmdl-bcc -lmdl-bci -lmdl-bitct
