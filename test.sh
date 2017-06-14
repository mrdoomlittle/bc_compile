make && ./bc_compile main.bc a.out
cd ../bc_interp
make && ./main ../bc_compile/a.out
