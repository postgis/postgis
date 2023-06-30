/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * Copyright (C) 2022 Martin Davis
 * Copyright 2008 Kevin Neufeld
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "styles.h"


void
getStyles( const char *filename, LAYERSTYLE **headRef )
{
	char line [128];
	FILE* pFile;
	char *getResults;

	*headRef = NULL;

	if ((pFile = fopen(filename, "r")) == NULL)
	{
		perror ( filename );
		return;
	}

	getResults = fgets ( line, sizeof line, pFile );
	while ( getResults != NULL )
	{

		// process defined styles
		while ( (getResults != NULL) && strncmp(line, "[Style]", 7) == 0)
		{
			char *styleName = "DefaultStyle";
			int pointSize = 5;
			char *pointColor = "Grey";
			int lineWidth = 5;
			char *lineColor = "Grey";
			int lineStartSize = 0;
			int lineEndSize = 0;
			int lineArrowSize = 0;
			char *polygonFillColor = "Grey";
			char *polygonStrokeColor = "Grey";
			int polygonStrokeWidth = 0;

			getResults = fgets ( line, sizeof line, pFile );
			while ( (getResults != NULL) && (strncmp(line, "[Style]", 7) != 0) )
			{
				char *ptr;

				// loop over all lines until [Style] is reached again
				if ( (*line != '#') && (ptr = strchr(line, '=')) )
				{
					ptr = trim((++ptr));

					if (strncmp(line, "styleName", 9) == 0)
						styleName = ptr;
					else if (strncmp(line, "pointSize", 9) == 0)
					{
						pointSize = atoi(ptr);
						free(ptr);
					}
					else if (strncmp(line, "pointColor", 10) == 0)
						pointColor = ptr;
					else if (strncmp(line, "lineWidth", 9) == 0)
					{
						lineWidth = atoi(ptr);
						free(ptr);
					}
					else if (strncmp(line, "lineColor", 9) == 0)
						lineColor = ptr;
					else if (strncmp(line, "lineWidth", 9) == 0)
					{
						lineWidth = atoi(ptr);
						free(ptr);
					}
					else if (strncmp(line, "lineStartSize", 13) == 0)
					{
						lineStartSize = atoi(ptr);
						free(ptr);
					}
					else if (strncmp(line, "lineEndSize", 11) == 0)
					{
						lineEndSize = atoi(ptr);
						free(ptr);
					}
					else if (strncmp(line, "lineArrowSize", 3) == 0)
					{
						lineArrowSize = atoi(ptr);
						free(ptr);
					}
					else if (strncmp(line, "polygonFillColor", 16) == 0)
						polygonFillColor = ptr;
					else if (strncmp(line, "polygonStrokeColor", 18) == 0)
						polygonStrokeColor = ptr;
					else if (strncmp(line, "polygonStrokeWidth", 18) == 0)
					{
						polygonStrokeWidth = atoi(ptr);
						free(ptr);
					}
				}
				getResults = fgets ( line, sizeof line, pFile );
			}
			addStyle(headRef, styleName, pointSize, pointColor,
				lineWidth, lineColor, lineStartSize, lineEndSize, lineArrowSize,
				polygonFillColor, polygonStrokeColor, polygonStrokeWidth);
		}
		getResults = fgets ( line, sizeof line, pFile );
	}
	fclose( pFile );
}


void
freeStyles( LAYERSTYLE **headRef )
{
	LAYERSTYLE *curr = *headRef;
	LAYERSTYLE *next;

	while (curr != NULL)
	{
		next = curr->next;
		free(curr->styleName);
		free(curr->pointColor);
		free(curr->lineColor);
		free(curr->polygonFillColor);
		free(curr->polygonStrokeColor);
		free(curr);
		curr = next;
	}

	*headRef = NULL;
}


void
addStyle(
    LAYERSTYLE **headRef,
    char* styleName,
    int pointSize, char* pointColor,
    int lineWidth, char* lineColor,
	int lineStartSize, int lineEndSize, int lineArrowSize,
    char* polygonFillColor, char* polygonStrokeColor, int polygonStrokeWidth)
{
	LAYERSTYLE *style = malloc( sizeof(LAYERSTYLE) );

	style->styleName = styleName;
	style->pointSize = pointSize;
	style->pointColor = pointColor;
	style->lineWidth = lineWidth;
	style->lineColor = lineColor;
	style->lineStartSize = lineStartSize;
	style->lineEndSize = lineEndSize;
	style->lineArrowSize = lineArrowSize;
	style->polygonFillColor = polygonFillColor;
	style->polygonStrokeColor = polygonStrokeColor;
	style->polygonStrokeWidth = polygonStrokeWidth;
	style->next = *headRef;
	*headRef = style;
}


int
length( LAYERSTYLE *head )
{
	int count = 0;
	LAYERSTYLE *curr = head;

	while (curr != NULL)
	{
		count++;
		curr = curr->next;
	}

	return (count);
}


LAYERSTYLE*
getStyle( LAYERSTYLE *headRef, char* styleName )
{
	LAYERSTYLE *curr = headRef;

	while (curr != NULL && (strcmp(curr->styleName, styleName) != 0))
		curr = curr->next;

	return (curr);
}


char*
trim(char* str)
{
	int len;
	char* result;
	char* start = str;
	char* end = strchr(start, '\0');
	while (start<end && isspace(*start)) start++;
	while (start<end && isspace(*(end-1))) end--;
	len = end-start;
	result = malloc( len+1 );
	strncpy(result, start, len);
	result[len] = '\0';
	return result;
}
