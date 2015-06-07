#include <stdio.h>
#include <stdlib.h>

#include "mpc/mpc.h"

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

int number_of_nodes(mpc_ast_t *t) {
	if (t->children_num == 0) {return 1;}

	if (t->children_num >= 1) {
		int total = 1;

		for (int i = 0; i < t->children_num; i++){
			total += number_of_nodes(t->children[i]);
		}
		return total;
	}

	return -1;
}

typedef struct {
	int type;
	long num;
	int err;
}lval;

enum {LVAL_NUM, LVAL_ERR};
enum {LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM};

/* create a new number type lval */
lval lval_num(long x) {
	lval v;
	v.type =LVAL_NUM;
	v.num = x;

	return v;
}

/* creat a new err type lval */
lval lval_err(int x) {
	lval v;
	v.type = LVAL_ERR;
	v.err = x;

	return v;
}

/* print an lval */
void lval_print(lval v) {
	switch(v.type) {
	case LVAL_NUM: printf("%li", v.num); break;
	case LVAL_ERR:
		switch(v.err){
		case LERR_DIV_ZERO:	printf("Error: Division Bu zero!"); break;
		case LERR_BAD_OP: printf("Error: Invalid Operator!"); break;
		case LERR_BAD_NUM: printf("Error: Invalid Number!"); break;
		}
		break;
	}
}

void lval_println(lval v) {
	lval_print(v);
	putchar('\n');
}

lval eval_op(lval x, char *op, lval y)
{

	if (x.type == LVAL_ERR) {return x;}
	if (y.type == LVAL_ERR) {return y;}

	if(strcmp(op, "+") == 0) {return lval_num(x.num + y.num);};
	if(strcmp(op, "-") == 0) {return lval_num(x.num - y.num);};
	if(strcmp(op, "*") == 0) {return lval_num(x.num * y.num);};
	if(strcmp(op, "/") == 0) {return y.num == 0	? lval_err(LERR_DIV_ZERO) : lval_num(x.num/y.num);};
	
	if(strcmp(op, "%") == 0) {return lval_num(x.num % y.num);};
		if(strcmp(op, "^") == 0) {return lval_num(pow(x.num, y.num));};

	return lval_err(LERR_BAD_OP);
}

lval eval(mpc_ast_t *t)
{
	/* if tagged as number return it directly */
	if (strstr(t->tag, "number")) {
		errno = 0;
		long x = strtol(t->contents, NULL, 10);
		return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
	}

	/* the operator is always second child */
	char *op = t->children[1]->contents;

	/* we store the third child in x */
	lval x = eval(t->children[2]);

	/* iterate the remaining children and combing. */
	int i = 3;
	while(strstr(t->children[i]->tag, "expr")) {
		x = eval_op(x, op, eval(t->children[i]));
		i++;
	}

	return x;
}

int main(int argc, char **argv ){

	mpc_parser_t *Number = mpc_new("number");
	mpc_parser_t *Operator = mpc_new("operator");
	mpc_parser_t *Expr = mpc_new("expr");
	mpc_parser_t *Lispx = mpc_new("lispx");

	mpca_lang(MPCA_LANG_DEFAULT,
			  "\
number   : /-?[0-9]+/;							  \
operator : '+' | '-' | '*' | '/' | '%' | '^';	  \
expr     : <number> | '(' <operator> <expr>+ ')'; \
lispx    : /^/ <operator> <expr>+ /$/;			  \
", Number, Operator, Expr, Lispx);

	puts("lispx Version 0.0.1");
	puts("Press Ctrl+c to exit\n");

	while(1) {
		char *input = readline("lispx>");
		add_history(input);

		mpc_result_t r;
		if(mpc_parse("<stdin>", input, Lispx, &r)) {
			/* load ast from output */
			mpc_ast_t *a = r.output;
			printf("number_of_nodes: %i\n", number_of_nodes(a));
			printf("Tag: %s\n", a->tag);
			printf("Contents: %s\n", a->contents);
			printf("Number of children: %i\n", a->children_num);

			/* get first child */
			mpc_ast_t *c0 = a->children[0];
			printf("First Child Tag: %s\n", c0->tag);
			printf("First Child Contents: %s\n", c0->contents);
			printf("First Child Number of children: %i\n", c0->children_num);
			
			mpc_ast_print(r.output);

			/* eval */
			lval result = eval(r.output);
			lval_println(result);

			mpc_ast_delete(r.output);
		} else {
			mpc_err_print(r.error);
			mpc_err_delete(r.error);
		}
		
		free(input);
	}
	mpc_cleanup(4, Number, Operator, Expr, Lispx);

	return 0;
}
