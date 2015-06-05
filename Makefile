all: lispx.c
	gcc -std=c99 -Wall lispx.c mpc/mpc.c -lm -ledit -o lispx
