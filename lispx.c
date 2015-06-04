#include <stdio.h>

static char input[2048];

int main(int argc, char **argv ){
	puts("lispx Version 0.0.1");
	puts("Press Ctrl+c to exit\n");

	while(1) {
		fputs("lispx>", stdout);
		
		fgets(input, 2048, stdin);

		printf("No you're a %s", input);
	}
}
