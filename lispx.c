#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <string.h>

char *readline(char *prompt) {
	static char buffer[2048];

	fputs(prompt, stdout);
	fgets(buffer, 2048, stdin);
	char *cpy = malloc(strlen(buffer)+1);
	strcpy(cpy, buffer);
	cpy[strlen[cpy]-1] = '\0';

	return cpy;
}

void add_history(char *unused) {}
#else

#include <editline/readline.h>
#include <editline/history.h>

#endif


int main(int argc, char **argv ){
	puts("lispx Version 0.0.1");
	puts("Press Ctrl+c to exit\n");

	while(1) {
		char *input = readline("lispx>");

		add_history(input);

		printf("No you're a %s\n", input);

		free(input);
	}

	return 0;
}
