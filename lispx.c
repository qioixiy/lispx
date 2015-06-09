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

enum {LVAL_ERR, LVAL_NUM, LVAL_SYM, LVAL_SEXPR, LVAL_QEXPR};
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

/* a pointer to a new empty Qexpr lval */
lval *lval_qexpr(void) 
{
	lval *v = malloc(sizeof(lval));
	v->type = LVAL_QEXPR;
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
	case LVAL_QEXPR:
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
	if (strstr(t->tag, "qexpr"))  {x = lval_qexpr();}

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
	case LVAL_QEXPR: lval_expr_print(v, '{', '}'); break;
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

lval *lval_pop(lval *v, int i) {
	/* find the item at 'i' */
	lval *x = v->cell[i];

	/* shift memory after the item at 'i' over the top */
	memmove(&v->cell[i], &v->cell[i+1],
			sizeof(lval*) * (v->count-i-1));

	/* decrease the count of items int the list */
	v->count--;
	
	/* reallocate the memory used */
	v->cell = realloc(v->cell, sizeof(lval *) * v->count);

	return x;
}

lval *lval_take(lval *v, int i) {
	lval *x = lval_pop(v, i);
	lval_del(v);

	return x;
}

lval *builtin_op(lval *a, char *op) {
	/* ensure all arguments are numbers */
	for (int i = 0; i < a->count; i++) {
		if (a->cell[i]->type != LVAL_NUM) {
			lval_del(a);
			return lval_err("Cannot operate on non-number!");
		}
	}

	/* pop the first element */
	lval *x = lval_pop(a, 0);

	/* if no arguments and sub the preform unary negation */
	if (strcmp(op, "-") == 0 && a->count == 0) {
		x->num = -x->num;
	}

	/* while there are still elements remaining */
	while(a->count > 0) {
		/* pop the next element */
		lval *y = lval_pop(a, 0);

		if(strcmp(op, "+") == 0) {x->num += y->num;}
		if(strcmp(op, "-") == 0) {x->num -= y->num;}
		if(strcmp(op, "*") == 0) {x->num *= y->num;}
		if(strcmp(op, "%") == 0) {x->num %= y->num;}
		if(strcmp(op, "^") == 0) {x->num = pow(x->num, y->num);}
		if(strcmp(op, "/") == 0) {
			if (y->num == 0) {
				lval_del(x);
				lval_del(y);
				x = lval_err("Division By Zero!");
			}
			x->num /= y->num;
		}
		lval_del(y);
	}
	
	lval_del(a);
	return x;
}

#define LASSERT(args, cond, err) \
	if ( !(cond)) {lval_del(args); return lval_err(err);}

lval *builtin_head(lval *a) {
	/* check error conditions */
	LASSERT(a, a->count == 1,
			"Function 'head' passed too many arguments");
	LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
			"Function 'head' passed incorrect types!");
	LASSERT(a, a->cell[0]->count != 0,
			"Function 'head' passed {}!");

	/* otherwise take first argument */
	lval *v = lval_take(a, 0);

	while(v->count > 1) {
		lval_del(lval_pop(v, 1));
	}

	return v;
}
lval *builtin_tail(lval *a) {
	/* check error conditions */
	LASSERT(a, a->count == 1,
			"Function 'head' passed too many arguments");
	LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
			"Function 'head' passed incorrect types!");
	LASSERT(a, a->cell[0]->count != 0,
			"Function 'head' passed {}!");

	/* take first argument */
	lval *v = lval_take(a, 0);
	/* delete first element and return */
	lval_del(lval_pop(v, 0));

	return v;
}

lval *builtin_list(lval *a) {
	a->type = LVAL_QEXPR;
	return a;
}

lval *lval_eval(lval *v);
lval *builtin_eval(lval *a) {
	LASSERT(a, a->count == 1,
			"Function 'head' passed too many arguments");
	LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
			"Function 'head' passed incorrect types!");
	
	lval *x = lval_take(a, 0);
	x->type = LVAL_SEXPR;

	return lval_eval(x);
}

lval *lval_join(lval *x, lval *y) {
	/* for each cell in 'y' and it to 'x' */
	while(y->count) {
		x = lval_add(x, lval_pop(y, 0));
	}

	/* delete the empty 'y' and return 'x' */
	lval_del(y);

	return x;
}

lval *builtin_join(lval *a) {
	for (int i = 0; i < a->count; i++) {
		LASSERT(a, a->cell[i]->type == LVAL_QEXPR,
				"Funtion 'join' passed incorrect type.");
	}

	lval *x = lval_pop(a, 0);
	
	while(a->count) {
		x = lval_join(x, lval_pop(a, 0));
	}

	lval_del(a);

	return x;
}

lval *builtin(lval *a, char *func) {
	if(strcmp("list", func) == 0) {return builtin_list(a);}
	if(strcmp("head", func) == 0) {return builtin_head(a);}
	if(strcmp("tail", func) == 0) {return builtin_tail(a);}
	if(strcmp("join", func) == 0) {return builtin_join(a);}
	if(strcmp("eval", func) == 0) {return builtin_eval(a);}

	if(strstr("+-*/%^", func)) {return builtin_op(a, func);}

	lval_del(a);
	return lval_err("Unknown Funtcion!");
}

lval *lval_eval_sexpr(lval *v) {
	/* evaluate children */
	for (int i = 0; i < v->count; i++) {
		v->cell[i] = lval_eval(v->cell[i]);
	}

    /* error checking */
	for(int i = 0; i < v->count; i++) {
		if (v->cell[i]->type == LVAL_ERR) {
			return lval_take(v, i);
		}
	}

	/* empty expression */
	if (v->count == 0) {return v;}

	/* single expression */
	if (v->count == 1) {return lval_take(v, 0);}

	/* ensure first element is symbol */
	lval *f = lval_pop(v, 0);
	if (f->type != LVAL_SYM) {
		lval_del(f);
		lval_del(v);
		return lval_err("S-expression does not start with symbol!");
	}

	/* call builtin with operator */
	lval *result = builtin(v, f->sym);
	lval_del(f);
	
	return result;
}

lval *lval_eval(lval *v) {
	/* evaluate sexpressions */
	if (v->type == LVAL_SEXPR) {
		return lval_eval_sexpr(v);
	}

	/* all other lval types remain the same */
	return v;
}

int main(int argc, char **argv ){

	mpc_parser_t *Number = mpc_new("number");
	mpc_parser_t *Symbol = mpc_new("symbol");
	mpc_parser_t *Sexpr = mpc_new("sexpr");
	mpc_parser_t *Qexpr = mpc_new("qexpr");
	mpc_parser_t *Expr = mpc_new("expr");
	mpc_parser_t *Lispx = mpc_new("lispx");

	mpca_lang(MPCA_LANG_DEFAULT,
			  "\
number   : /-?[0-9]+/ ;											\
symbol   : '+' | '-' | '*' | '/' | '%' | '^'					\
		 | \"list\"| \"head\"| \"tail\"	| \"join\" | \"eval\";	\
sexpr    : '(' <expr>* ')' ;									\
qexpr    : '{' <expr>* '}' ;									\
expr     : <number> | <symbol> | <sexpr> | <qexpr> ;			\
lispx    : /^/ <expr>* /$/ ;									\
",
			  Number, Symbol, Sexpr, Qexpr, Expr, Lispx);

	puts("lispx Version 0.0.1");
	puts("Press Ctrl+c to exit\n");

	while(1) {
		char *input = readline("lispx>");
		add_history(input);

		mpc_result_t r;
		if(mpc_parse("<stdin>", input, Lispx, &r)) {
//#define DEBUG
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
			
			lval *x = lval_eval(lval_read(r.output));
			lval_println(x);
			lval_del(x);

			mpc_ast_delete(r.output);
		} else {
			mpc_err_print(r.error);
			mpc_err_delete(r.error);
		}
		
		free(input);
	}
	mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, Lispx);

	return 0;
}
