#include <stdio.h>
#include <stdlib.h>

#include "mpc/mpc.h"

mpc_parser_t *Number;
mpc_parser_t *Symbol;
mpc_parser_t *String;
mpc_parser_t *Comment;
mpc_parser_t *Sexpr;
mpc_parser_t *Qexpr;
mpc_parser_t *Expr;
mpc_parser_t *Lispx;

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
enum {LVAL_ERR, LVAL_NUM, LVAL_SYM, LVAL_STR,
	  LVAL_FUN, LVAL_SEXPR, LVAL_QEXPR,};
enum {LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM,};

typedef lval* (*lbuiltin)(lenv*, lval*);

struct lenv {
	lenv *par;
	int count;
	char **syms;
	lval **vals;
};

struct lval{
	int type;

	/* Basic */
	long num;
	char *err;/* Error and Symbol types have some string data */
	char *sym;
	char *str;

	/* Function */
	lbuiltin builtin;
	lenv *env;
	lval *formals;
	lval *body;

	/* Expression */
	int count;/* count and point to a list of "lval*" */
	lval **cell;
};

lval *lval_num(long x);
lval *lval_err(char *fmt, ...);
lval *lval_sym(char *m);
lval *lval_str(char *s);
lval *lval_sexpr(void);
lval *lval_qexpr(void);
lval *lval_lambda(lval *formals, lval *body);
lval *lval_fun(lbuiltin func);
lval *lval_read_num(mpc_ast_t *t);
lval *lval_read_str(mpc_ast_t *t);
lval *lval_add(lval *v, lval *x);
lval *lval_read(mpc_ast_t *t);
lval *lval_pop(lval *v, int i);
lval *lval_take(lval *v, int i);
lval *lval_copy(lval *v);
lval *lval_call(lenv *e, lval *f, lval *a);
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
lval *builtin_put(lenv *e, lval *a);
lval *builtin_var(lenv *e, lval *a, char *func);
lval *builtin_lambda(lenv *e, lval *a);
lval *builtin_ord(lenv *e, lval *a, char *op);
lval *builtin_gt(lenv *e, lval *a);
lval *builtin_lt(lenv *e, lval *a);
lval *builtin_ge(lenv *e, lval *a);
lval *builtin_le(lenv *e, lval *a);
lval *builtin_cmp(lenv *e, lval *a, char *op);
lval *builtin_eq(lenv *e, lval *a);
lval *builtin_ne(lenv *e, lval *a);
lval *builtin_if(lenv *e, lval *a);
lval *builtin_load(lenv *e, lval *a);
lval *builtin_print(lenv *e, lval *a);
lval *builtin_error(lenv *e, lval *a);
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
int  lval_eq(lval *x, lval *y);
lenv *lenv_new(void);
void lenv_del(lenv *e);
lval *lenv_get(lenv *e, lval *k);
lenv *lenv_copy(lenv *e);
void lenv_put(lenv *e, lval *k, lval *v);
void lenv_def(lenv *e, lval *k, lval *v);

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
	case LVAL_STR: return "String";
	case LVAL_SEXPR: return "S-Expression";
	case LVAL_QEXPR: return "Q-Expression";
	default: return "Unknown";
	}
}

int  lval_eq(lval *x, lval *y) {
	/* Different Types are always unequal */
	if (x->type != y->type) {return 0;}

	/* Compare Based upon type */
	switch(x->type) {
		/*Compare Number Value*/
	case LVAL_NUM: return (x->num == y->num);
		/*Compare String Value*/
	case LVAL_ERR: return (strcmp(x->err, y->err) == 0);
	case LVAL_SYM: return (strcmp(x->sym, y->sym) == 0);
	case LVAL_STR: return (strcmp(x->str, y->str) == 0);
		/*If builtin Compare, otherwise Compare formals and body*/
	case LVAL_FUN:
		if (x->builtin || y->builtin) {
			return x->builtin == y->builtin;
		} else {
			return lval_eq(x->formals, y->formals)
				&& lval_eq(x->body, y->body);
		}
		/*If list Compare every individual element*/
	case LVAL_QEXPR:
	case LVAL_SEXPR:
		if (x->count != y->count) {return 0;}
		for (int i = 0; i < x->count; i++) {
			/*if any element not equal then whole list not equal*/
			if (!lval_eq(x->cell[i], y->cell[i])) {return 0;}
		}
		/*otherwise lists must be equal*/
		return 1;
		break;
	}

	return 0;
}

lenv *lenv_new(void) {
	lenv *e = malloc(sizeof(lenv));
	e->par = NULL;
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

	/* if no symbol check in parent otherwise error */
	if (e->par) {
		return lenv_get(e->par, k);
	} else {
		/* if no symbol found return error */
		return lval_err("unbound symbol %s!", k->sym);
	}
}

lenv *lenv_copy(lenv *e) {
	lenv *n = malloc(sizeof(lenv));

	n->par = e->par;
	n->count = e->count;
	n->syms = malloc(sizeof(char *) * n->count);
	n->vals = malloc(sizeof(lval *) * n->count);

	for (int i = 0; i < e->count; i++) {
		n->syms[i] = malloc(strlen(e->syms[i]) + 1);
		strcpy(n->syms[i], e->syms[i]);
		n->vals[i] = lval_copy(e->vals[i]);
	}

	return n;
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

void lenv_def(lenv *e, lval *k, lval *v) {
	/* Iterate till e has no parent */
	while(e->par) {
		e = e->par;
	}

	/* put value in e */
	lenv_put(e, k, v);
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

lval *lval_lambda(lval *formals, lval *body)
{
	lval *v = malloc(sizeof(lval));
	v->type = LVAL_FUN;

	/* set Builtin to NULL */
	v->builtin = NULL;

	/* Build new environment */
	v->env = lenv_new();

	/* Set Formals and Body */
	v->formals = formals;
	v->body = body;

	return v;
}

lval *lval_fun(lbuiltin func) {
	lval *v = malloc(sizeof(lval));

	v->type = LVAL_FUN;
	v->builtin = func;

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

lval *lval_str(char *s)
{
	lval *v = malloc(sizeof(lval));

	v->type = LVAL_STR;
	v->str = malloc(strlen(s)+1);
	strcpy(v->str, s);

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
		if (!v->builtin) {
			lenv_del(v->env);
			lval_del(v->formals);
			lval_del(v->body);
		}
		break;
	case LVAL_NUM: break;
	case LVAL_ERR: free(v->err); break;
	case LVAL_SYM: free(v->sym); break;
	case LVAL_STR: free(v->str); break;
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
	lenv_add_builtin(e, "\\", builtin_lambda);
	lenv_add_builtin(e, "def", builtin_def);
	lenv_add_builtin(e, "=",   builtin_put);

	/* Comparison functions */
	lenv_add_builtin(e, "if", builtin_if);
	lenv_add_builtin(e, "==", builtin_eq);
	lenv_add_builtin(e, "!=", builtin_ne);
	lenv_add_builtin(e, ">", builtin_gt);
	lenv_add_builtin(e, "<", builtin_lt);
	lenv_add_builtin(e, ">=", builtin_ge);
	lenv_add_builtin(e, "<=", builtin_le);

	/* String functions */
	lenv_add_builtin(e, "load", builtin_load);
	lenv_add_builtin(e, "error", builtin_error);
	lenv_add_builtin(e, "print", builtin_print);
}

lval *lval_read_num(mpc_ast_t *t) {
	errno = 0;
	long x = strtol(t->contents, NULL, 10);
	return errno != ERANGE ?
		lval_num(x) : lval_err("invalid number");
}

lval *lval_read_str(mpc_ast_t *t) {
	/*Cut off the final quote character*/
	t->contents[strlen(t->contents)-1] = '\0';
	/*copy the string missing out the first quote character*/
	char *unescaped = malloc(strlen(t->contents+1)+1);
	strcpy(unescaped, t->contents+1);
	/*pass throuth the unescaped function*/
	unescaped = mpcf_unescape(unescaped);
	/*construct a new lval using the string*/
	lval *str = lval_str(unescaped);

	/*free the string and return*/
	free(unescaped);
	return str;
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
	if(strstr(t->tag, "string")) {return lval_read_str(t);}
	
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
		if(strstr(t->children[i]->tag, "comment")) {continue;}
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

void lval_print_str(lval *v) {
	/*Make a copy of the string*/
	char *escaped = malloc(strlen(v->str)+1);
	strcpy(escaped, v->str);
	/*Pass it through the escape function*/
	escaped = mpcf_escape(escaped);
	/*Print it between " characters*/
	printf("\"%s\"", escaped);
	/*free the copied string*/
	free(escaped);
}

void lval_print(struct lval *v) {
	switch(v->type) {
	case LVAL_NUM: printf("%li", v->num); break;
	case LVAL_ERR: printf("Error: %s", v->err); break;
	case LVAL_FUN:
		if (v->builtin) {
			printf("<builtin>");
		} else {
			printf("(\\ "); lval_print(v->formals);
			putchar(' '); lval_print(v->body); putchar(')');
		}
		break;
	case LVAL_SYM: printf("%s", v->sym); break;
	case LVAL_STR: lval_print_str(v); break;
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
	case LVAL_FUN:
		if (v->builtin) {/* for builtin*/
			x->builtin = v->builtin;
		} else {
			x->builtin = NULL;
			x->env = lenv_copy(v->env);
			x->formals = lval_copy(v->formals);
			x->body = lval_copy(v->body);
		}
		break;
	case LVAL_NUM: x->num = v->num; break;

		/* copy string using malloc and strcpy */
	case LVAL_ERR:
		x->err = malloc(strlen(v->err) + 1);
		strcpy(x->err, v->err); break;
	case LVAL_SYM:
		x->sym = malloc(strlen(v->sym) + 1);
		strcpy(x->sym, v->sym); break;
	case LVAL_STR:
		x->str = malloc(strlen(v->str) + 1);
		strcpy(x->str, v->str); break;

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

lval *lval_call(lenv *e, lval *f, lval *a) {
	/* If Builtin then simply call that */
	if (f->builtin) {
		return f->builtin(e, a);
	}

	/* Record Argument Counts */
	int given = a->count;
	int total = f->formals->count;
	
	/* while argument still remain to be processed */
	while(a->count) {
		/* If we've ran out of formal arguments to bind */
		if (f->formals->count == 0) {
			lval_del(a);
			return lval_err(
				"Function passed too many arguments. "
				"Got %i, Expected %i.", given, total);
		}
		
		/* Pop the first symbol from the formals */
		lval *sym = lval_pop(f->formals, 0);

		/* Special Case to deal with '&' */
		if (strcmp(sym->sym, "&") == 0) {
			/* Ensure '&' is followed by another symbol */
			if (f->formals->count != 1) {
				lval_del(a);
				return lval_err("Function format invalid. "
								"Symbol '&' not followed by single symbol.");
			}

			/* Next formal should be bound to remaining arguments */
			lval *nsym = lval_pop(f->formals, 0);
			lenv_put(f->env, nsym, builtin_list(e, a));
			lval_del(sym);
			lval_del(nsym);
			break;
		}

		/* Pop the next argment from the list */
		lval *val = lval_pop(a, 0);

		/* Bind a copy into the function's environment */
		lenv_put(f->env, sym, val);
		/* Delete symbol and value */
		lval_del(sym);
		lval_del(val);
	}

	/* Argument list is now bound so can be cleaned up */
	lval_del(a);

	/* If '&' remains in formal list bind to empty list */
	if (f->formals->count > 0
		&& strcmp(f->formals->cell[0]->sym, "&") == 0) {
		/* Check to ensure that & is not passed invalidly. */
		if (f->formals->count != 2) {
			return lval_err("Function format invalid. "
							"Symbol '&' not followed by single symbol.");
		}

		/* Pop and delete '&' symbol */
		lval_del(lval_pop(f->formals, 0));

		/* Pop next symbol and create empty list */
		lval *sym = lval_pop(f->formals, 0);
		lval *val = lval_qexpr();

		/* Bind to environment and delete */
		lenv_put(f->env, sym, val);
		lval_del(sym);
		lval_del(val);
	}

	/* If all formals have been bound evalute */
	if (f->formals->count == 0) {
		/* Set environment parent to evaluation environment */
		f->env->par = e;

		/* Evaluate and return */
		return builtin_eval(
			f->env, lval_add(lval_sexpr(), lval_copy(f->body)));
	} else {
		/* Otherwise return partially evaluated function */
		return lval_copy(f);
	}
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
	return builtin_var(e, a, "def");
}

lval *builtin_put(lenv *e, lval *a) {
	return builtin_var(e, a, "=");
}

lval *builtin_var(lenv *e, lval *a, char *func) {
	LASSERT_TYPE(func, a, 0, LVAL_QEXPR);

	lval *syms = a->cell[0];
	for (int i = 0; i < syms->count; i++) {
		LASSERT(a, (syms->cell[i]->type == LVAL_SYM),
				"Function '%s' cannot define non-symbol. "
				"Got %s, Expected %s.", func,
				ltype_name(syms->cell[i]->type),
				ltype_name(LVAL_SYM));
	}

	LASSERT(a, (syms->count == a->count-1),
		"Function '%s' passed too many arguments for symbols. "
		"Got %i, Expected %i.", func, syms->count, a->count-1);

	for (int i = 0; i < syms->count; i++) {
		/* If 'def' define in globally. If 'put' define in locally*/
		if (strcmp(func, "def") == 0) {
			lenv_def(e, syms->cell[i], a->cell[i+1]);
		}

		if (strcmp(func, "=") == 0) {
			lenv_put(e, syms->cell[i], a->cell[i+1]);
		}
	}

	lval_del(a);

	return lval_sexpr();
}

lval *builtin_lambda(lenv *e, lval *a) {
	/* check two arguments, each of which are Q-Expressions */
	LASSERT_NUM("\\", a, 2);
	LASSERT_TYPE("\\", a, 0, LVAL_QEXPR);
	LASSERT_TYPE("\\", a, 1, LVAL_QEXPR);

	/* check first Q-Expression contains only Symbols */
	for (int i = 0; i < a->cell[0]->count; i++) {
		LASSERT(a, (a->cell[0]->cell[i]->type == LVAL_SYM),
				"Cannot define non-symbol. Got %s, Expected %s.",
				ltype_name(a->cell[0]->cell[i]->type),
				ltype_name(LVAL_SYM));
	}

	/* Pop first two arguments and pass them to lval_lambda */
	lval *formals = lval_pop(a, 0);
	lval *body = lval_pop(a, 0);
	lval_del(a);

	return lval_lambda(formals, body);
}

lval *builtin_ord(lenv *e, lval *a, char *op)
{
	LASSERT_NUM(op, a, 2);
	LASSERT_TYPE(op, a, 0, LVAL_NUM);
	LASSERT_TYPE(op, a, 1, LVAL_NUM);

	int r;
	if (strcmp(op, ">") == 0) {
		r = (a->cell[0]->num > a->cell[1]->num);
	}
	if (strcmp(op, "<") == 0) {
		r = (a->cell[0]->num < a->cell[1]->num);
	}
	if (strcmp(op, ">=") == 0) {
		r = (a->cell[0]->num >= a->cell[1]->num);
	}
	if (strcmp(op, "<=") == 0) {
		r = (a->cell[0]->num <= a->cell[1]->num);
	}

	lval_del(a);
	return lval_num(r);
}

lval *builtin_gt(lenv *e, lval *a)
{
	return builtin_ord(e, a, ">");
}

lval *builtin_lt(lenv *e, lval *a)
{
	return builtin_ord(e, a, "<");
}

lval *builtin_ge(lenv *e, lval *a)
{
	return builtin_ord(e, a, ">=");
}

lval *builtin_le(lenv *e, lval *a)
{
	return builtin_ord(e, a, "<=");
}

lval *builtin_cmp(lenv *e, lval *a, char *op)
{
	LASSERT_NUM(op, a, 2);

	int r;
	if (strcmp(op, "==")  == 0) {
		r = lval_eq(a->cell[0], a->cell[1]);
	}
	if (strcmp(op, "!=")  == 0) {
		r = !lval_eq(a->cell[0], a->cell[1]);
	}
	lval_del(a);
	return lval_num(r);
}

lval *builtin_eq(lenv *e, lval *a)
{
	return builtin_cmp(e, a, "==");
}

lval *builtin_ne(lenv *e, lval *a)
{
	return builtin_cmp(e, a, "!=");
}

lval *builtin_if(lenv *e, lval *a)
{
	LASSERT_NUM("if", a, 3);
	LASSERT_TYPE("if", a, 0, LVAL_NUM);
	LASSERT_TYPE("if", a, 1, LVAL_QEXPR);
	LASSERT_TYPE("if", a, 2, LVAL_QEXPR);

	/*Mark Both Expressions as evaluable*/
	lval *x;
	a->cell[1]->type = LVAL_SEXPR;
	a->cell[2]->type = LVAL_SEXPR;

	if (a->cell[0]->num) {
		/*if conditions is true evaluate first Expression*/
		x = lval_eval(e, lval_pop(a, 1));
	} else {
		/*otherwise evaluate second Expression*/
		x = lval_eval(e, lval_pop(a, 2));
	}

	/*Delete argument list and return*/
	lval_del(a);
	return x;
}

lval *builtin_print(lenv *e, lval *a) {
	/* Print each argument followed by a space */
	for (int i = 0; i < a->count; i++) {
		lval_print(a->cell[i]); putchar(' ');
	}

	/* Print a newline and delete arguments */
	putchar('\n');
	lval_del(a);

	return lval_sexpr();
}

lval *builtin_error(lenv *e, lval *a) {
	LASSERT_NUM("error", a, 1);
	LASSERT_TYPE("error", a, 0, LVAL_STR);

	/* Construct Error from first argument */
	lval *err = lval_err(a->cell[0]->str);

	/* Delete arguments and return */
	lval_del(a);

	return err;
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
	lval *result = lval_call(e, f, v);

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

lval *builtin_load(lenv *e, lval *a) {
	LASSERT_NUM("load", a, 1);
	LASSERT_TYPE("load", a, 0, LVAL_STR);

	/* Parse File given by string name */
	mpc_result_t r;
	if (mpc_parse_contents(a->cell[0]->str, Lispx, &r)) {

		/* Read contents */
		lval *expr = lval_read(r.output);
		mpc_ast_delete(r.output);

		/* Evaluate each Expression */
		while(expr->count) {
			lval *x = lval_eval(e, lval_pop(expr, 0));
			/* If Evaluation leads to error print it  */
			if (x->type == LVAL_ERR) {lval_println(x);}
			lval_del(x);
		}

		/* Delete expressions and arguments */
		lval_del(expr);
		lval_del(a);

		/* Return empty list */
		return lval_sexpr();
	} else {
		/* Get Parse Error as String */
		char *err_msg = mpc_err_string(r.error);
		mpc_err_delete(r.error);

		/* Create new error message using it */
		lval *err = lval_err("Could not load library %s", err_msg);
		free(err_msg);
		lval_del(a);

		/* Cleanup and return error */
		return err;
	}
}

int main(int argc, char **argv )
{

	Number = mpc_new("number");
	Symbol = mpc_new("symbol");
	String = mpc_new("string");
	Comment = mpc_new("comment");
	Sexpr = mpc_new("sexpr");
	Qexpr = mpc_new("qexpr");
	Expr = mpc_new("expr");
	Lispx = mpc_new("lispx");

	mpca_lang(MPCA_LANG_DEFAULT,
			  "\
number   : /-?[0-9]+/ ;											\
symbol   : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&]+/ ;					\
string   : /\"(\\\\.|[^\"])*\"/ ;								\
comment  : /;[^\\r\\n]*/ ;										\
sexpr    : '(' <expr>* ')' ;									\
qexpr    : '{' <expr>* '}' ;									\
expr     : <number> | <symbol> | <string>						\
         | <comment> | <sexpr> | <qexpr> ;						\
lispx    : /^/ <expr>* /$/ ;									\
",
			  Number, Symbol, String, Comment, Sexpr, Qexpr, Expr, Lispx);

	puts("lispx Version 0.0.1");
	puts("Press Ctrl+c to exit\n");
	
	lenv *e = lenv_new();
	lenv_add_builtins(e);

	/* Supplied with list of files */
	if (argc >= 2) {
		/* loop over each supplied filename (starting from 1) */
		for (int i = 1; i < argc; i++) {
			/*Argument list with a single argument, the filename */
			lval *args = lval_add(lval_sexpr(), lval_str(argv[1]));

			/*Pass to builtin load and get the result*/
			lval *x = builtin_load(e, args);

			/*If the result is an error be sure to print it*/
			if (x->type == LVAL_ERR) {lval_println(x);}
			lval_del(x);
		}
	}

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
	mpc_cleanup(8,
				Number, Symbol, String, Comment,
				Sexpr, Qexpr, Expr, Lispx);

	return 0;
}
