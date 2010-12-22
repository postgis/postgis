%{
/* MapAlgebra parser */
#include <stdio.h>
#include <stdlib.h>

%}

%union
{
    double real;
    int integer;
	char *id;
}

%token <real> TKN_REAL;
%token <integer> TKN_INTEGER;
%token <id> TKN_ID;

%start ma_calc
%left '+' '-'
%left '*' '/'

%%

ma_calc:			statement_list;

statement_list:		statement | 
					statement_list ';' statement
					;
					
statement:			factor {} |
					statement '+' statement {} |
					statement '-' statement {} |
					statement '*' statement {} |
					statement '/' statement {} |
					'(' statement ')'
					;

factor:				TKN_ID {} |
					TKN_REAL {} |
					TKN_INTEGER {}
					;

%%

void yyerror(char *s)
{
    fprintf(stderr, "Error %s\n", s);
}

int main(int argc, char **argv)
{
    if (argc > 1)
        yyin=fopen(argv[1], "rt");
    else
        yyin=stdin;

    return yyparse();
}

/* To compile
   bison -d ma_parser.y
   flex ma_lexer.l
   gcc lex.yy.c ma_parser.tab.c -o ma_parser -lfl -lm
   */
