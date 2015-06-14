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

/* forward declartions */

struct lval;
struct lenv;

typedef struct lval lval;
typedef struct lenv lenv;

/* lisp value */
enum {LVAL_ERR, LVAL_NUM, LVAL_SYM,
	  LVAL_FUN, LVAL_SEXPR, LVAL_QEXPR,};
enum {LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM,};

typedef lval* (*lbuiltin)(lenv*, lval*);

struct lenv {
	int count;
	char **syms;
	lval **vals;
};

struct lval{
	int type;

	long num;

	/* Error and Symbol types have some string data */
	char *err;
	char *sym;

	lbuiltin fun;

	/* count and point to a list of "lval*" */
	int count;
	lval **cell;
};

lval *lval_num(long x);
lval *lval_err(char *fmt, ...);
lval *lval_sym(char *m);
lval *lval_sexpr(void);
lval *lval_qexpr(void);
lval *lval_fun(lbuiltin func);
lval *lval_read_num(mpc_ast_t *t);
lval *lval_add(lval *v, lval *x);
lval *lval_read(mpc_ast_t *t);
lval *lval_pop(lval *v, int i);
lval *lval_take(lval *v, int i);
lval *lval_copy(lval *v);
lval *builtin_head(lenv *e, lval *a);
lval *builtin_tail(lenv *e, lval *a);
lval *builtin_list(lenv *e, lval *a);
lval *builtin_len(lenv *e, lval *a);
lval *builtin_eval(lenv *e, lval *a);
lval *builtin_join(lenv *e, lval *a);
lval *builtin_add(lenv *e,lval *a);
lval *builtin_sub(lenv *e,lval *a);
lval *builtin_mul(lenv *e,lval *a);
lval *builtin_div(lenv *e,lval *a);
lval *builtin_op(lenv *e, lval *a, char *op);
lval *builtin_def(lenv *e, lval *a);
lval *builtin(lenv *e, lval *a, char *func);
lval *lval_join(lval *x, lval *y);
lval *lval_eval_sexpr(lenv *e, lval *v);
lval *lval_eval(lenv *e, lval *v);
void lenv_add_builtin(lenv *e, char *name, lbuiltin func);
void lenv_add_builtins(lenv *e);
void lval_del(struct lval *v);
void lval_expr_print(lval *v, char open, char close);
void lval_print(struct lval *v);
void lval_println(struct lval *v);
char *ltype_name(int t);

lenv *lenv_new(void);
void lenv_del(lenv *e);
lval *lenv_get(lenv *e, lval *k);

#define LASSERT(args, cond, fmt, ...)				\
	if ( !(cond)) {									\
		lval *err = lval_err(fmt, ##__VA_ARGS__);	\
		lval_del(args);								\
		return err;									\
	}
#define LASSERT_TYPE(func, args, index, expect)						\
	LASSERT(args, args->cell[index]->type == expect,				\
			"Function '%s' passed incorrect type for argument %i. "	\
			"Got %s, Expected %s.",									\
			func, index, ltype_name(args->cell[index]->type),		\
			ltype_name(expect))
#define LASSERT_NUM(func, args, num)								\
	LASSERT(args, args->count == num,								\
			"Function '%s', passed incorrect number of argumen. "	\
			"Got %i, Expected %i.",									\
			func, args->count, num)
#define LASSERT_NOT_EMPTY(func, args, index)			\
	LASSERT(args, args->cell[index]->count != 0,		\
			"Function '%s' passed {} for argument %i.",	\
			func, index)

/* function */
char *ltype_name(int t) {
	switch(t) {
	case LVAL_FUN: return "Function";
	case LVAL_NUM: return "Number";
	case LVAL_ERR: return "Error";
	case LVAL_SYM: return "Symbol";
	case LVAL_SEXPR: return "S-Expression";
	case LVAL_QEXPR: return "Q-Expression";
	default: return "Unknown";
	}
}

lenv *lenv_new(void) {
	lenv *e = malloc(sizeof(lenv));
	e->count = 0;
	e->syms = NULL;
	e->vals = NULL;

	return e;
}

void lenv_del(lenv *e) {
	for (int i = 0; i < e->count; i++) {
		free(e->syms[i]);
		lval_del(e->vals[i]);
	}

	free(e->syms);
	free(e->vals);

	free(e);
}

lval *lenv_get(lenv *e, lval *k) {
	/* iterate over all items in environment */
	for (int i = 0; i < e->count; i++) {
		/* check if the stored string matches the symbol string
		   if it does, return a copy of the value */
		if (strcmp(e->syms[i], k->sym) == 0){
			return lval_copy(e->vals[i]);
		}
	}

	/* if no symbol found return error */
	return lval_err("unbound symbol %s!", k->sym);
}

void lenv_put(lenv *e, lval *k, lval *v) {
	/* iterate over all items in environment
	   this is to see if variable already exists */
	for (int i = 0; i < e->count; i++) {
		/* if variable is found delete item at that postion
		   and replace with variable supplied by user */
		if (strcmp(e->syms[i], k->sym) == 0){
			lval_del(e->vals[i]);
			e->vals[i] = lval_copy(v);
			return;
		}
	}
	/* if no existing entry found allocate space for new entry */
	e->count++;
	e->vals = realloc(e->vals, sizeof(lval*) * e->count);
	e->syms = realloc(e->syms, sizeof(char*) * e->count);

	/* copy content of lval and symbol string into new location */
	e->vals[e->count-1] = lval_copy(v);
	e->syms[e->count-1] = malloc(strlen(k->sym)+1);
	strcpy(e->syms[e->count-1], k->sym);
}

/* construct a pointer to a new number lval */
lval *lval_num(long x) {
	lval *v = malloc(sizeof(lval));
	v->type = LVAL_NUM;
	v->num = x;

	return v;
}

/* construct a pointer to a new err lval */
lval *lval_err(char *fmt, ...) {
	lval *v = malloc(sizeof(lval));
	v->type = LVAL_ERR;

	/* create a va list and initialize it */
	va_list va;
	va_start(va, fmt);

	/* allocate 512 bytes of space */
	v->err = malloc(512);

	/* printf the error string with a maximum of 511 characters */
	vsnprintf(v->err, 511, fmt, va);

	/* reallocate to number of bytes actually used */
	v->err = realloc(v->err, strlen(v->err)+1);

	/* cleanup our va list */
	va_end(va);

	return v;
}

lval *lval_fun(lbuiltin func) {
	lval *v = malloc(sizeof(lval));

	v->type = LVAL_FUN;
	v->fun = func;

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
	case LVAL_FUN:
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

void lenv_add_builtin(lenv *e, char *name, lbuiltin func) {
	lval *k = lval_sym(name);
	lval *v = lval_fun(func);

	lenv_put(e, k, v);
	lval_del(k);
	lval_del(v);
}

void lenv_add_builtins(lenv *e) {
	/* list function */
	lenv_add_builtin(e, "list", builtin_list);
	lenv_add_builtin(e, "head", builtin_head);
	lenv_add_builtin(e, "tail", builtin_tail);
	lenv_add_builtin(e, "eval", builtin_eval);
	lenv_add_builtin(e, "join", builtin_join);

	/* mathematical functions */
	lenv_add_builtin(e, "+", builtin_add);
	lenv_add_builtin(e, "-", builtin_sub);
	lenv_add_builtin(e, "*", builtin_mul);
	lenv_add_builtin(e, "/", builtin_div);

	/* variable function */
	lenv_add_builtin(e, "def", builtin_def);
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
	case LVAL_FUN: printf("<function>"); break;
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

lval *lval_copy(lval *v) {

	lval *x = malloc(sizeof(lval));
	x->type = v->type;

	switch(v->type) {
		/* copy function and numbers directly */
	case LVAL_FUN: x->fun = v->fun; break;
	case LVAL_NUM: x->num = v->num; break;

		/* copy string using malloc and strcpy */
	case LVAL_ERR:
		x->err = malloc(strlen(v->err) + 1);
		strcpy(x->err, v->err); break;
	case LVAL_SYM:
		x->sym = malloc(strlen(v->sym) + 1);
		strcpy(x->sym, v->sym); break;

		/* copy lists by copying each sub-expression */
	case LVAL_SEXPR:
	case LVAL_QEXPR:
		x->count = v->count;
		x->cell = malloc(sizeof(lval*) * x->count);
		for (int i = 0; i < x->count; i++) {
			x->cell[i] = lval_copy(v->cell[i]);
		}
		break;
	}

	return x;
}

lval *builtin_op(lenv *e, lval *a, char *op) {
	/* ensure all arguments are numbers */
	for (int i = 0; i < a->count; i++) {
		LASSERT_TYPE(op, a, i, LVAL_NUM);
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

lval *builtin_def(lenv *e, lval *a) {
	LASSERT_TYPE("def", a, 0, LVAL_QEXPR);

	/* first argument is symbol list */
	lval *syms = a->cell[0];

	/* ensure all elements of first list are symbols */
	for (int i = 0; i < syms->count; i++) {
		LASSERT(a, syms->cell[i]->type == LVAL_SYM,
				"Function 'def' cannot define non-symbol."
				"Got %s, Expected %s.",
				ltype_name(syms->cell[i]->type),
				ltype_name(LVAL_SYM));
	}

	/* check correct number of symbols and values */
	LASSERT(a, syms->count == a->count-1,
			"Function 'def' passed too many arguments for symbols. "
			"Got %i, Expected %i.",
			syms->count, a->count-1);

	/* assign copies of values to symbols */
	for (int i = 0; i < syms->count; i++) {
		lenv_put(e, syms->cell[i], a->cell[i+1]);
	}

	lval_del(a);
	return lval_sexpr();
}

lval *builtin_head(lenv *e, lval *a) {
	/* check error conditions */
	LASSERT_NUM("head", a, 1);
	LASSERT_TYPE("head", a, 0, LVAL_QEXPR);
	LASSERT_NOT_EMPTY("head", a, 0);

	/* otherwise take first argument */
	lval *v = lval_take(a, 0);

	while(v->count > 1) {
		lval_del(lval_pop(v, 1));
	}

	return v;
}
lval *builtin_tail(lenv *e, lval *a) {
	/* check error conditions */
	LASSERT_NUM("tail", a, 1);
	LASSERT_TYPE("tail", a, 0, LVAL_QEXPR);
	LASSERT_NOT_EMPTY("tail", a, 0);

	/* take first argument */
	lval *v = lval_take(a, 0);
	/* delete first element and return */
	lval_del(lval_pop(v, 0));

	return v;
}

lval *builtin_list(lenv *e, lval *a) {
	a->type = LVAL_QEXPR;
	return a;
}

lval *builtin_eval(lenv *e, lval *a) {
	LASSERT_NUM("eval", a, 1);
	LASSERT_TYPE("eval", a, 0, LVAL_QEXPR);
	
	lval *x = lval_take(a, 0);
	x->type = LVAL_SEXPR;

	return lval_eval(e, x);
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

lval *builtin_join(lenv *e, lval *a) {
	for (int i = 0; i < a->count; i++) {
		LASSERT_TYPE("join", a, i, LVAL_QEXPR);
	}

	lval *x = lval_pop(a, 0);
	
	while(a->count) {
		x = lval_join(x, lval_pop(a, 0));
	}

	lval_del(a);

	return x;
}

lval *builtin_len(lenv *e, lval *a) {
	LASSERT(a, a->count == 1,
			"Function 'head' passed too many arguments");
	LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
			"Funtion 'len' passed incorrect type.");
	
	long count = a->cell[0]->count;
	lval_del(a);
	return lval_num(count);
}

lval *builtin_add(lenv *e,lval *a) {
	return builtin_op(e, a, "+");
}

lval *builtin_sub(lenv *e,lval *a) {
	return builtin_op(e, a, "-");
}

lval *builtin_mul(lenv *e,lval *a) {
	return builtin_op(e, a, "*");
}

lval *builtin_div(lenv *e,lval *a) {
	return builtin_op(e, a, "/");
}

lval *builtin(lenv *e, lval *a, char *func) {
	if(strcmp("list", func) == 0) {return builtin_list(e, a);}
	if(strcmp("head", func) == 0) {return builtin_head(e, a);}
	if(strcmp("tail", func) == 0) {return builtin_tail(e, a);}
	if(strcmp("join", func) == 0) {return builtin_join(e, a);}
	if(strcmp("eval", func) == 0) {return builtin_eval(e, a);}
	if(strcmp("len", func) == 0)  {return builtin_len(e, a);}

	if(strstr("+-*/", func)) {return builtin_op(e, a, func);}

	lval_del(a);
	return lval_err("Unknown Funtcion!");
}

lval *lval_eval_sexpr(lenv *e, lval *v) {
	/* evaluate children */
	for (int i = 0; i < v->count; i++) {
		v->cell[i] = lval_eval(e, v->cell[i]);
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

	/* ensure first element is a function after evaluation */
	lval *f = lval_pop(v, 0);
	if (f->type != LVAL_FUN) {
		lval *err = lval_err("S-Expression starts with incorrect type. "
							 "Got %i, Expected %s.",
							 ltype_name(f->type),
							 ltype_name(LVAL_FUN));
		lval_del(f);
		lval_del(v);

		return err;
	}

	/* if so call function to get result */
	lval *result = f->fun(e, v);
	lval_del(f);
	
	return result;
}

lval *lval_eval(lenv *e, lval *v) {
	if (v->type == LVAL_SYM) {
		lval *x = lenv_get(e, v);
		lval_del(v);
		return x;
	}
	
	/* evaluate sexpressions */
	if (v->type == LVAL_SEXPR) {
		return lval_eval_sexpr(e, v);
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
symbol   : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&]+/ ;					\
sexpr    : '(' <expr>* ')' ;									\
qexpr    : '{' <expr>* '}' ;									\
expr     : <number> | <symbol> | <sexpr> | <qexpr> ;			\
lispx    : /^/ <expr>* /$/ ;									\
",
			  Number, Symbol, Sexpr, Qexpr, Expr, Lispx);

	puts("lispx Version 0.0.1");
	puts("Press Ctrl+c to exit\n");
	
	lenv *e = lenv_new();
	lenv_add_builtins(e);

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
			
			lval *x = lval_eval(e, lval_read(r.output));
			lval_println(x);
			lval_del(x);

			mpc_ast_delete(r.output);
		} else {
			mpc_err_print(r.error);
			mpc_err_delete(r.error);
		}
		
		free(input);
	}
	lenv_del(e);
	mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, Lispx);

	return 0;
}
