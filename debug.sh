rm -f main.rbc
sh compile.sh && ./bin/bcc -o main.rbc -i main.bc -I lib && bcd main.rbc
