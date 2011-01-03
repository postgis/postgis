%{
#include <stdlib.h>
#include "ma_parser.tab.h"

static YY_BUFFER_STATE ma_yy_buf_state;


/*
* Set up the lexer!
*/
void ma_lexer_init(char *src)
{
	ma_yy_buf_state = yy_scan_string(src);
}

/*
* Clean up the lexer!
*/
void ma_lexer_close()
{
	yy_delete_buffer(ma_yy_buf_state);
}

%}

integer		-?[0-9]+
real		-?(([0-9]+\.?)|([0-9]*\.?[0-9]+)([eE][-+]?[0-9]+)?)
symbol		"+"|"-"|"*"|"/"|"^"|"("|")"
%%

{integer}	{yylval.integer = atoi(yytext); return TKN_INTEGER;}
{real}		{yylval.real = atof(yytext); return TKN_REAL;}
{symbol}	{return yytext[0];}
.			;
%%