%{

/* MapAlgebra lexer */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ma_parser.tab.h"

#define TOK_ERROR -1

int nlines = 0;

%}
%option noyywrap
space		[ \t]
integer		[0-9]+
real		[0-9]*"."[0-9]+
symbol		"+"|"-"|"*"|"/"|"("|")"
id			[a-zA-Z_][0-9a-zA-Z_]*

%%
{space}			;
{integer}		{yylval.integer = atoi(yytext); return TKN_INTEGER;}
{real}			{yylval.real = atof(yytext); return TKN_REAL;}
{symbol}		{return yytext[0];}
{id}			{yylval.id = strdup(yytext); return TKN_ID;}
.				{fprintf(stderr, "Bad syntax in ma_lexer\n"); return TOK_ERROR;}

%%

void main(int argc, char **argv)
{
    if (argc > 1)
        yyin = fopen(argv[1], "rt");
    else
        yyin = stdin;
    yylex();
}


/* To compile:
   flex ma_lexer.l
   gcc lex.yy.c -o ma_lexer -lfl -lm
   */
