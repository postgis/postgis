%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

void yyerror(char *);
%}

%union {
	int integer;
	double real;
}

%token <integer> TKN_INTEGER
%token <real> TKN_REAL
%type <integer> integer_exp
%type <real> real_exp

%left '-' '+'
%left '*' '/'
%left NEG
%right '^'

%start calc

%%

calc:
	| calc line
    ;

line:
	integer_exp { printf("%d\n", $1); }
	| real_exp { printf("%f\n", $1); }
    ;

integer_exp:
        TKN_INTEGER                     { $$ = $1;          }
		| integer_exp '+' integer_exp   { $$ = $1 + $3;     }
		| integer_exp '-' integer_exp   { $$ = $1 - $3;     }
		| integer_exp '*' integer_exp   { $$ = $1 * $3;     }               
		| integer_exp '^' integer_exp   { $$ = pow($1, $3); }
		| '(' integer_exp ')'           { $$ = $2;          }
		;

real_exp:
        TKN_REAL                        { $$ = $1;          }
		| real_exp '+' real_exp         { $$ = $1 + $3;     }
		| real_exp '-' real_exp         { $$ = $1 - $3;     }
		| real_exp '*' real_exp         { $$ = $1 * $3;     }
		| real_exp '/' real_exp         { $$ = $1 / $3;     }
		| real_exp '^' real_exp         { $$ = pow($1, $3); }
		| '(' real_exp ')'              { $$ = $2;          }
		;


%%

/* Just for testing purposes */
int main(int argc, char **argv)
{
    char * testStr = (char*)calloc(10, sizeof(char));
    memcpy(testStr, argv[1], 10*sizeof(char));

    ma_lexer_init(testStr);

    yyparse();

    ma_lexer_close();

    free(testStr);

    return 0;
}

void yyerror(char *msg)
{
  fprintf(stderr, "Error: %s\n", msg);
}

/* To compile:
    bison -d ma_parser.y
    flex ma_parser.l
    gcc lex.yy.c ma_parser.tab.c -o ma_parser -lfl -lm
*/