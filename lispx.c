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


typedef struct lval{
	int type;
	long num;

	/* Error and Symbol types have some string data */
	char *err;
	char *sym;

	/* count and point to a list of "lval*" */
	int count;
	struct lval **cell;
}lval;

enum {LVAL_ERR, LVAL_NUM, LVAL_SYM, LVAL_SEXPR};
enum {LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM};

/* construct a pointer to a new number lval */
lval *lval_num(long x) {
	lval *v = malloc(sizeof(lval));
	v->type = LVAL_NUM;
	v->num = x;

	return v;
}

/* construct a pointer to a new err lval */
lval *lval_err(char *m) {
	lval *v = malloc(sizeof(lval));

	v->type = LVAL_ERR;
	v->err = malloc(strlen(m)+1);
	strcpy(v->err, m);

	return v;
}

/* construct a pointer to a new symbol lval */
lval *lval_sym(char *m)
{
	lval *v = malloc(sizeof(lval));

	v->type = LVAL_SYM;
	v->sym = malloc(strlen(m)+1);
	strcpy(v->sym, m);

	return v;
}

/* a pointer to a new empty sexpr lval */
lval *lval_sexpr(void)
{
	lval *v = malloc(sizeof(lval));
	v->type = LVAL_SEXPR;
	v->count = 0;
	v->cell=NULL;

	return v;
}

void lval_del(struct lval *v)
{
	switch(v->type) {
	case LVAL_NUM: break;
	case LVAL_ERR: free(v->err); break;
	case LVAL_SYM: free(v->sym); break;
	case LVAL_SEXPR:
		for(int i = 0; i < v->count; i++) {
			lval_del(v->cell[i]);
		}
		free(v->cell);
		break;
	}

	free(v);
}

lval *lval_read_num(mpc_ast_t *t) {
	errno = 0;
	long x = strtol(t->contents, NULL, 10);
	return errno != ERANGE ?
		lval_num(x) : lval_err("invalid number");
}

lval *lval_add(lval *v, lval *x) {
	v->count++;
	v->cell = realloc(v->cell, sizeof(lval *) * v->count);
	v->cell[v->count-1]=x;

	return v;
}

lval *lval_read(mpc_ast_t *t) {
	/* if symbol or number return conversion to that type */
	if(strstr(t->tag, "number")) {return lval_read_num(t);}
	if(strstr(t->tag, "symbol")) {return lval_sym(t->contents);}
	
	/* if root (>) or sexpr then create empty list */
	lval *x = NULL;
	if (strcmp(t->tag, ">") == 0) {x = lval_sexpr();}
	if (strstr(t->tag, "sexpr"))  {x = lval_sexpr();}

	/* fill this list with any valid expression contained within */
	for (int i = 0; i < t->children_num; i++) {
		if (strcmp(t->children[i]->contents, "(") == 0) {continue;}
		if (strcmp(t->children[i]->contents, ")") == 0) {continue;}
		if (strcmp(t->children[i]->contents, "{") == 0) {continue;}
		if (strcmp(t->children[i]->contents, "}") == 0) {continue;}
		if (strcmp(t->children[i]->tag,  "regex") == 0) {continue;}
		x = lval_add(x, lval_read(t->children[i]));
	}

	return x;
}

void lval_print(struct lval *v);
void lval_expr_print(lval *v, char open, char close) {
	putchar(open);
	for(int i = 0; i < v->count; i++) {
		/* print value contained within */
		lval_print(v->cell[i]);

		/* don't print trailing space if last element */
		if (i != (v->count-1)) {
			putchar(' ');
		}
	}

	putchar(close);
}

void lval_print(struct lval *v) {
	switch(v->type) {
	case LVAL_NUM: printf("%li", v->num); break;
	case LVAL_ERR: printf("Error: %s", v->err); break;
	case LVAL_SYM: printf("%s", v->sym); break;
	case LVAL_SEXPR: lval_expr_print(v, '(', ')'); break;
	}
}

void lval_println(struct lval *v) {
	lval_print(v);
	putchar('\n');
}

#if 0
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
#endif

int main(int argc, char **argv ){

	mpc_parser_t *Number = mpc_new("number");
	mpc_parser_t *Symbol = mpc_new("symbol");
	mpc_parser_t *Sexpr = mpc_new("sexpr");
	mpc_parser_t *Expr = mpc_new("expr");
	mpc_parser_t *Lispx = mpc_new("lispx");

	mpca_lang(MPCA_LANG_DEFAULT,
			  "\
number   : /-?[0-9]+/ ;							  \
symbol   : '+' | '-' | '*' | '/' | '%' | '^' ;	  \
sexpr    : '(' <expr>* ')' ;					  \
expr     : <number> | <symbol> | <sexpr> ;		  \
lispx    : /^/ <expr>* /$/ ;			  \
",
			  Number, Symbol, Sexpr, Expr, Lispx);

	puts("lispx Version 0.0.1");
	puts("Press Ctrl+c to exit\n");

	while(1) {
		char *input = readline("lispx>");
		add_history(input);

		mpc_result_t r;
		if(mpc_parse("<stdin>", input, Lispx, &r)) {

#ifdef DEBUG
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
#endif
			
			lval *v = lval_read(r.output);
			lval_println(v);
			lval_del(v);

			mpc_ast_delete(r.output);
		} else {
			mpc_err_print(r.error);
			mpc_err_delete(r.error);
		}
		
		free(input);
	}
	mpc_cleanup(5, Number, Symbol, Sexpr, Expr, Lispx);

	return 0;
}
