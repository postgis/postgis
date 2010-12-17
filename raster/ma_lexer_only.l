%{

/* MapAlgebra lexer */
#include <stdio.h>
#include <stdlib.h>

int nlines = 0;

%}

DIGIT [0-9]
ID [a-zA-Z][a-zA-Z0-9_]*

%%

{DIGIT}+            {printf("Found TKN_INTEGER: %d\n", atoi(yytext));}
{DIGIT}+"."{DIGIT}+ {printf("Found TKN_REAL: %d\n", atoi(yytext));}
"="                 {printf("Found TKN_ASSIGN: %s\n",yytext);}
";"                 {printf("Found TKN_SEMICOLON: %s\n",yytext);}
"*"                 {printf("Found TKN_PROD: %s\n",yytext);}
"/"                 {printf("Found TKN_DIV: %s\n",yytext);}
"+"                 {printf("Found TKN_PLUS: %s\n",yytext);}
"-"                 {printf("Found TKN_MINUS: %s\n",yytext);}
"("                 {printf("Found TKN_LFT_PAR: %s\n",yytext);}
")"                 {printf("Found TKN_RGT_PAR: %s\n",yytext);}
"cos"               {printf("Found TKN_COS: %s\n",yytext);}
"sin"               {printf("Found TKN_SIN: %s\n",yytext);}
"\n"                {nlines++;}
{ID}                {printf("Found TKN_ID: %s\n", yytext);}
. 

%%

void main(int argc, char **argv)
{
    if (argc > 1)
        yyin = fopen(argv[1], "rt");
    else
        yyin = stdin;
    yylex();
    printf("%d lines analyzed\n", nlines);
}

/* To compile:
   flex ma_lexer_only.l
   gcc lex.yy.c -o ma_lexer_only -lfl -lm
   */
