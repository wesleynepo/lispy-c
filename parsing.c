#include <stdio.h>
#include <stdlib.h>
#include "mpc.h"

#ifdef _WIN32
#include <string.h>

static char buffer[2048];

/* Fake readline function */
char* readline(char* prompt) {
  fputs(prompt, stdout);
  fgets(buffer, 2048, stdin);
  char* cpy = malloc(strlen(buffer)+1);
  strcpy(cpy, buffer);
  cpy[strlen(cpy)-1] = '\0';
  return cpy;
}

/* Fake add_history function */
void add_history(char* unused) {}

/* Otherwise include the editline headers */
#else
#include <editline/readline.h>
#include <editline/history.h>
#endif


#include <editline/readline.h>
#include <editline/history.h>

enum { LVAL_NUM, LVAL_ERR };
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };
typedef struct {
  int type;
  union {
    double num;
    int err;
  };
} lval;


lval lval_num(double x) {
  lval v;
  v.type = LVAL_NUM;
  v.num = x;
  return v;
}

lval lval_err(int x) {
  lval v;
  v.type = LVAL_ERR;
  v.err = x;
  return v;
}

void lval_print(lval v) {
  switch (v.type) {
    case LVAL_NUM: printf("%f", v.num); break;

    case LVAL_ERR:
      if (v.err == LERR_DIV_ZERO) {
        printf("Error: Division By Zero!");
      }
      if (v.err == LERR_BAD_OP) {
        printf("Error: Invalid Number!");
      }
      if (v.err == LERR_BAD_NUM) {
        printf("Error: Invalid operator!");
      }
    break;
  }
}

void lval_println(lval v) { lval_print(v); putchar('\n'); }

lval evaluate_operation(lval x, char* operator, lval y) {
  if (strcmp(operator, "+") == 0) { return lval_num(x.num + y.num); }
  if (strcmp(operator, "-") == 0) { return lval_num(x.num - y.num); }
  if (strcmp(operator, "*") == 0) { return lval_num(x.num * y.num); }
  if (strcmp(operator, "%") == 0) { return lval_num(fmod(x.num, y.num)); }
  if (strcmp(operator, "^") == 0) { return lval_num(pow(x.num, y.num)); }
  if (strcmp(operator, "min") == 0) { return lval_num((x.num < y.num)? x.num : y.num); }
  if (strcmp(operator, "max") == 0) { return lval_num((x.num > y.num)? x.num : y.num); }

  if (strcmp(operator, "/") == 0) { 
    return y.num == 0
      ? lval_err(LERR_DIV_ZERO)
      : lval_num(x.num / y.num);
  }

  return lval_err(LERR_BAD_OP);
}

lval evaluate(mpc_ast_t* t) {

  if (strstr(t->tag, "number")) {
    errno = 0;
    double x = strtod(t->contents, NULL);
    return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
  }
  
  char* operator = t->children[1]->contents;
  
  lval x = evaluate(t->children[2]);

  int i = 3;

  while (strstr(t->children[i]->tag, "expr")) {
    x = evaluate_operation(x, operator, evaluate(t->children[i]));
    i++;
  }

  if(t->children_num == 4 && strcmp(operator, "-") == 0) {
    return lval_num(-x.num);
  }

  return x;
}

int main(int argc, char** argv) {

  mpc_parser_t* Number = mpc_new("number");
  mpc_parser_t* Operator = mpc_new("operator");
  mpc_parser_t* Expr = mpc_new("expr");
  mpc_parser_t* Lispy = mpc_new("lispy");

  mpca_lang(MPCA_LANG_DEFAULT,
  "                                                                 \
    number   : /-?[0-9]+(\\.[0-9]+)?/ ;                                         \
    operator : '+' | '-' | '*' | '/' | '%' | '^' | /min/ | /max/ ;  \
    expr     : <number> | '(' <operator> <expr>+ ')' ;              \
    lispy    : /^/ <operator> <expr>+ /$/ ;                         \
  ",
  Number, Operator, Expr, Lispy);

  puts("Lispy Version 0.0.0.1");
  puts("Press Ctrl + C to Exit \n");

  while(1) {
    char* input = readline("lispy> ");

    add_history(input);

    mpc_result_t r;
    if (mpc_parse("<stdin>", input, Lispy, &r)) {
      lval result = evaluate(r.output);
      lval_println(result);
      mpc_ast_delete(r.output);
    } else {
    /* Otherwise Print the Error */
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }

    free(input);
  }

  mpc_cleanup(4, Number, Operator, Expr, Lispy);
  return 0;
}
