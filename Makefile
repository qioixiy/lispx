all: lispx.c
	gcc -g -std=c99 -Wall lispx.c mpc/mpc.c -lm -ledit -o lispx
