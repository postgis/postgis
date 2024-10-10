/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 * Copyright 2008 Kevin Neufeld
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 * TODO: add descriptions
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct layerStyle LAYERSTYLE;

struct layerStyle
{
	char *styleName; // A unique name

	int	  pointSize;
	char *pointColor;

	int   lineWidth;
	char *lineColor;

	int   lineStartSize;
	int   lineEndSize;
	int   lineArrowSize;

	char *polygonFillColor;
	char *polygonStrokeColor;
	int   polygonStrokeWidth;

	LAYERSTYLE *next;
};

void getStyles( const char *filename, LAYERSTYLE **headRef );
void freeStyles( LAYERSTYLE **headRef );
void addStyle( LAYERSTYLE **headRef, char* styleName, int pointSize, char* pointColor,
	int lineWidth, char* lineColor, int lineStartSize, int lineEndSize, int lineArrowSize,
	char* polygonFillColor, char* polygonStrokeColor, int polygonStrokeWidth );

int length( LAYERSTYLE *headRef );
LAYERSTYLE* getStyle( LAYERSTYLE *headRef, char* styleName );
char* trim(char* str);
