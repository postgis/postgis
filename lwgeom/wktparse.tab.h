#ifndef BISON_WKTPARSE_TAB_H
# define BISON_WKTPARSE_TAB_H

#ifndef YYSTYPE
typedef union {
	double value;
	const char* wkb;
} yystype;
# define YYSTYPE yystype
#endif
# define	POINT	257
# define	LINESTRING	258
# define	POLYGON	259
# define	MULTIPOINT	260
# define	MULTILINESTRING	261
# define	MULTIPOLYGON	262
# define	GEOMETRYCOLLECTION	263
# define	SRID	264
# define	EMPTY	265
# define	VALUE	266
# define	LPAREN	267
# define	RPAREN	268
# define	COMMA	269
# define	EQUALS	270
# define	SEMICOLON	271
# define	WKB	272


extern YYSTYPE lwg_parse_yylval;

#endif /* not BISON_WKTPARSE_TAB_H */
