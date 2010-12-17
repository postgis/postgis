%{

/* MapAlgebra lexer */
#include <stdio.h>
#include <stdlib.h>
#include "ma_parser.tab.h"

int nlines = 0;

%}

DIGIT [0-9]
ID [a-zA-Z][a-zA-Z0-9_]*

%%

{DIGIT}+("."{DIGIT}+)?  {//printf("Found TKN_REAL: %d\n", atoi(yytext));
                        yylval.real = atof(yytext);
                        return TKN_REAL;}
"="                 {//printf("Found TKN_ASSIGN: %s\n",yytext);
                    return TKN_ASSIGN;}
";"                 {//printf("Found TKN_SEMICOLON: %s\n",yytext);
                    return TKN_SEMICOLON;}
"*"                 {//printf("Found TKN_PROD: %s\n",yytext);
                    return TKN_PROD;}
"/"                 {//printf("Found TKN_DIV: %s\n",yytext);
                    return TKN_DIV;}
"+"                 {//printf("Found TKN_PLUS: %s\n",yytext);
                    return TKN_PLUS;}
"-"                 {//printf("Found TKN_MINUS: %s\n",yytext);}
                    return TKN_MINUS;
"("                 {//printf("Found TKN_LFT_PAR: %s\n",yytext);
                    return TKN_LFT_PAR;}
")"                 {//printf("Found TKN_RGT_PAR: %s\n",yytext);
                    return TKN_RGT_PAR;}
"cos"               {//printf("Found TKN_COS: %s\n",yytext);
                    return TKN_COS;}
"sin"               {//printf("Found TKN_SIN: %s\n",yytext);
                    return TKN_SIN;}
"\n"                {nlines++;}
{ID}                {//printf("Found TKN_ID: %s\n", yytext);
                    return TKN_ID;}
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
   flex ma_lexer.l
   gcc lex.yy.c -o ma_lexer -lfl -lm
   */
