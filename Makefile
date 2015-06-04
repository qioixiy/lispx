all: lispx.c
	gcc -std=c99 -Wall lispx.c -ledit -o lispx
