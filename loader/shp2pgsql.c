/**********************************************************************
 * $Id$
 *
 * Author: Jeff Lounsbury, jeffloun@refractions.net
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2001-2003 Refractions Research Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of hte GNU General Public Licence. See the COPYING file.
 * 
 **********************************************************************
 * $Log$
 * Revision 1.45  2004/01/02 20:11:41  strk
 * always call setval with no schema specification. drop 'database' argument using the empty string to the AddGeometryColumn call
 *
 * Revision 1.44  2003/12/30 13:31:53  strk
 * made shp2pgsql looser about numeric precisions
 *
 * Revision 1.43  2003/12/30 12:37:46  strk
 * Fixed segfault bug reported by Randy George, removed explicit sequence drop
 *
 * Revision 1.42  2003/12/01 14:27:58  strk
 * added simple malloc wrapper
 *
 * Revision 1.41  2003/09/29 16:15:22  pramsey
 * Patch from strk:
 * - "\t" always preceeded the first value of a dump_format query
 *   if NULL
 *
 * - field values where quoted with (") in dump_format when
 *   called with -k ( did I introduce that? )
 *
 * - Appropriate calls to DBF[..]ReadAttributes based on
 *   cached attribute types.
 *
 * - Assured that *all* shapes are NULL before exiting with
 *   an error ( I did not check that NULL shapes in the midle
 *   of the shapefiles are handled, but previous code did
 *   not check that either ... )
 *
 * Revision 1.40  2003/09/19 00:37:33  jeffloun
 * fixed a bug that actually tests the first 2 point for pip instead of just thinking I was testing the first two.
 *
 * Revision 1.39  2003/08/17 19:00:14  pramsey
 * Change sequence handling to respect versions prior to 7.3. Patch from
 * strk.
 *
 * Revision 1.38  2003/08/05 16:28:05  jeffloun
 * Removed the setval for the sequence if the value was going to be 0.
 * This avoids a warning that occirs when you try to set it to 0.
 *
 * Revision 1.37  2003/08/01 23:22:44  jeffloun
 * Altered the loader to use a (gid serial) type instead of just a (gid int4).
 * Also the gid is now declared as a primary key.
 *
 * Revision 1.36  2003/07/01 18:30:55  pramsey
 * Added CVS revision headers.
 *
 * Revision 1.35  2003/06/18 16:30:56  pramsey
 * It seems that invalid geometries where in the shapefile (as far as shapelib
 * let shp2pgsql know). LINEZ objects with less then 2 vertices. I've
 * patched shp2pgsql to recognized such an inconsistence and use a NULL
 * geometry for that record printing a warning on stderr.
 * <strk@freek.keybit.net>
 *
 * Revision 1.34  2003/04/14 18:01:42  pramsey
 * Patch for optional case sensitivity respect. From strk.
 *
 * Revision 1.33  2003/04/01 23:02:50  jeffloun
 *
 * Fixed a bug which dropped the last Z value of each line in 3d lines.
 *
 * Revision 1.32  2003/03/07 16:39:53  pramsey
 * M-handling patch and some Z-recognition too.
 * From strk@freek.keybit.net.
 *
 * Revision 1.31  2003/02/15 00:27:14  jeffloun
 * added more type checking into the create table statment.
 * Now uses int8, and numeric types if the columns definitions are too big
 *
 * Revision 1.30  2003/02/14 20:07:26  jeffloun
 * changed the PIP function to loop from i=0 to  1<n-1
 *
 * Revision 1.29  2003/02/04 22:57:44  pramsey
 * Fix memory management error, array of pointers allocated insufficient space.
 *
 * Revision 1.28  2003/02/04 21:39:20  pramsey
 * Added CVS substitution strings for logging.
 *
 **********************************************************************
 * Using shapelib 1.2.8, this program reads in shape files and 
 * processes it's contents into a Insert statements which can be 
 * easily piped into a database frontend.
 * Specifically designed to insert type 'geometry' (a custom 
 * written PostgreSQL type) for the shape files and PostgreSQL 
 * standard types for all attributes of the entity. 
 **********************************************************************/

#include "shapefil.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "getopt.h"

typedef struct {double x, y, z;} Point;

typedef struct Ring{
	Point *list;	//list of points
	struct Ring  *next;
	int		n;		//number of points in list
} Ring;

int	dump_format = 0; //0=insert statements, 1 = dump
int	quoteidentifiers = 0;
char    opt;
char    *col_names;

DBFFieldType *types;	/* Fields type, width and precision */
int 	*widths;
int 	*precisions;

int Insert_attributes(DBFHandle hDBFHandle, int row);
char	*make_good_string(char *str);
int ring_check(SHPObject* obj, char *table, char *sr_id, int rings,
DBFHandle hDBFHandle);
char	*protect_quotes_string(char *str);
int PIP( Point P, Point* V, int n );
void *safe_malloc(size_t size);

void *safe_malloc(size_t size)
{
	void *ret = malloc(size);
	if ( ! ret ) {
		fprintf(stderr, "Out of virtual memory\n");
		exit(1);
	}
	return ret;
}

#define malloc(x) safe_malloc(x)


char	*make_good_string(char *str){
	//find all the tabs and make them \<tab>s
	//
	// 1. find # of tabs
	// 2. make new string 
	//
	// we dont escape already escaped tabs


	char	*result;
	char	*str2;
	char	*start,*end;
	int	num_tabs = 0;

	(str2) = (str);

	while ((str2 = strchr(str2, '\t')) )
	{
		if ( (str2 == str) || (str2[-1] != '\\') ) //the previous char isnt a '\'
			num_tabs ++;
		str2++;
	}
	if (num_tabs == 0)
		return str;
	
	result =(char *) malloc ( strlen(str) + num_tabs+1);
	memset(result,0, strlen(str) + num_tabs+1 );
	start = str;

	while((end = strchr((start),'\t')))
	{
		if ( (end == str) || (end[-1] != '\\' ) ) //the previous char isnt a '\'
		{
			strncat(result, start, (end-start));	
			strcat(result,"\\\t");
			start = end +1;
		}
		else
		{
			strncat(result, start, (end-start));	
			strcat(result,"\t");	
			start = end  +1;
		}
	}
	strcat(result,start);
	return result;
	
}

char	*protect_quotes_string(char *str){
	//find all quotes and make them \quotes
	//find all '\' and make them '\\'
	// 
	// 1. find # of characters
	// 2. make new string 

	char	*result;
	char	*str2;
	char	*start, *end1, *end2 = NULL;
	int	num_chars = 0;

	str2 =  str;

	while ((str2 = strchr((str2), '\'')) )
	{
		//if ( (str2 == str) || (str2[-1] != '\\') ) //the previous char isnt a '\'
		num_chars ++;
		str2++;
	}
	
	str2 = str;
	
	while ((str2 = strchr((str2), '\\')) )
	{
		num_chars ++;
		str2++;
	}
	if (num_chars == 0)
		return str;
	
	result =(char *) malloc ( strlen(str) + num_chars+1);
	memset(result,0, strlen(str) + num_chars+1 );
	start = str;

	while((end1 = strchr((start),'\''))||(end2 = strchr((start),'\\')))
	{
	   
	  if(end1){
	    strncat(result, start, (end1-start));
	    strcat(result, "\\\'");
	    start = end1+1;
	  }else{
	    strncat(result, start, (end2-start));
	    strcat(result, "\\\\");
	    start = end2+1;
	  }
	    
	  end1 = 0;
	}
	    
	strcat(result,start);
	return result;
}



// PIP(): crossing number test for a point in a polygon
//      input:   P = a point,
//               V[] = vertex points of a polygon V[n+1] with V[n]=V[0]
//      returns: 0 = outside, 1 = inside
int PIP( Point P, Point* V, int n ){
    int cn = 0;    // the crossing number counter
	int i;
    // loop through all edges of the polygon
    for (i=0; i<n-1; i++) {    // edge from V[i] to V[i+1]
       if (((V[i].y <= P.y) && (V[i+1].y > P.y))    // an upward crossing
        || ((V[i].y > P.y) && (V[i+1].y <= P.y))) { // a downward crossing
            double vt = (float)(P.y - V[i].y) / (V[i+1].y - V[i].y);
            if (P.x < V[i].x + vt * (V[i+1].x - V[i].x)) // P.x < intersect
                ++cn;   // a valid crossing of y=P.y right of P.x
        }
    }
    return (cn&1);    // 0 if even (out), and 1 if odd (in)

}


//This function basically deals with the polygon case.
//it sorts the polys in order of outer,inner,inner, so that inners always come after outers they are within 
//return value is the number of rings seen so far, used to keep id's unique.

int ring_check(SHPObject* obj, char *table, char *sr_id, int rings,DBFHandle hDBFHandle){
	Point pt,pt2;
	Ring *Poly;
	Ring *temp;
	Ring **Outer; //pointer to a list of outer polygons
	Ring **Inner; //pointer to a list of inner polygons
	int out_index,in_index,indet_index; //indexes to the lists **Outer and **Inner
	int	in,temp2;
	int u,i,N,n;
	int next_ring;	//the index of the panPartStart list 
	double area;	
	
	//initialize all counters/indexes
	out_index=0;
	in_index=0;
	indet_index=0;
	area=0;
	n=0;
	i=0;
	N = obj->nVertices;

	if(obj->nParts >1){
		next_ring = 1;//if there is more than one part, set the next_ring index to one
	}else{
		next_ring = -99;
	}


	//allocate initial pointer memory
	Outer = (Ring**)malloc(sizeof(Ring*)*obj->nParts);
	Inner = (Ring**)malloc(sizeof(Ring*)*obj->nParts);
	Poly = (Ring*)malloc(sizeof(Ring));
	Poly->list = (Point*)malloc(sizeof(Point)*N);
	Poly->next = NULL;


	for (u=0;u<N;u++){

		//check if the next point is the start of a new ring
		if( ((next_ring != -99) && (u+1 == obj->panPartStart[next_ring] )) || u==N-1){
			//check if a ring is clockwise(outer) or not(inner) by getting positive(inner) or negative(outer) area.
			//'area' is actually twice actual polygon area so divide by 2, not that it matters but in case we use it latter...
			area = area/2.0;
			if(area < 0.0 || obj->nParts ==1){

				//An outer ring so fill in the last point then put it in the 'Outer' list
				Poly->list[n].x = obj->padfX[u]; //the polygon is ended with it's first co-ordinates reused
				Poly->list[n].y = obj->padfY[u];
				Poly->list[n].z = obj->padfZ[u];
				Poly->n = n+1;
				Outer[out_index] = Poly;
				out_index++;

				if(u != N-1){ //dont make another ring if we are finished
					//allocate memory to start building the next ring
					Poly = (Ring*)malloc(sizeof(Ring));

					//temp2 is the number of points in the list of the next ring
					//determined so that we can allocate the right amount of mem 6 lines down
					if((next_ring + 1) == obj->nParts){
						temp2 = N;
					}else{
						temp2 = obj->panPartStart[next_ring+1] - obj->panPartStart[next_ring];
					}
					Poly->list = (Point*)malloc(sizeof(Point)*temp2);
					Poly->next = NULL;//make sure to make to initiale next to null or you never know when the list ends
								  //this never used to be here and was a pain in the ass bug to find...
				}
				n=0;//set your count of what point you are at in the current ring back to 0

			}else{

				Poly->list[n].x = obj->padfX[u]; //the polygon ends with it's first co-ordinates reused
				Poly->list[n].y = obj->padfY[u];
				Poly->list[n].z = obj->padfZ[u];
				Poly->n = n+1;

				Inner[in_index] = Poly;
				in_index++;

				Poly = (Ring*)malloc(sizeof(Ring));
				temp2 = N;
				if((next_ring + 1) == obj->nParts){
				}else{
					temp2 = obj->panPartStart[next_ring+1] - obj->panPartStart[next_ring];
				}


				Poly->list = (Point*)malloc(sizeof(Point)*temp2);
				Poly->next = NULL;

				n=0;

			}
			area=0.0;
			if((next_ring + 1) == obj->nParts){
			}else{
				next_ring++;
			}
		}else{

			Poly->list[n].x = obj->padfX[u];
			Poly->list[n].y = obj->padfY[u];
			Poly->list[n].z = obj->padfZ[u];
			n++;
			area += (obj->padfX[u] * obj->padfY[u+1]) - (obj->padfY[u] * obj->padfX[u+1]); //calculate the area

		}
	}



	//Put the inner rings into the list of the outer rings of which they are within
	for(u=0; u < in_index; u++){
		pt.x = Inner[u]->list[0].x;
		pt.y = Inner[u]->list[0].y;

		pt2.x = Inner[u]->list[1].x;
		pt2.y = Inner[u]->list[1].y;
		for(i=0;i< out_index; i++){
			in = PIP(pt,Outer[i]->list,Outer[i]->n);
			if(in==1 || (PIP(pt2,Outer[i]->list,Outer[i]->n) == 1)){
				Poly = Outer[i];
				while(Poly->next != NULL){
					Poly = Poly->next;
				}
				Poly->next = Inner[u];
				break;
			}
		}
		//if the ring wasn't within any outer rings, assume it is a new outer ring

		if(i == out_index){
			Outer[out_index] = Inner[u];
			out_index++;
		}
	}

	//start spitting out the sql for ordered entities now.
	if (dump_format){
		if(opt != 'a'){
			printf("%i\t",rings);
		}
	}else{
		if ( quoteidentifiers ){
			if(opt == 'a'){
				printf("\nInsert into \"%s\" %s values(",table,col_names);
			}else{
				printf("\nInsert into \"%s\" %s values('%i',",table,col_names,rings);
			}
		}else{
			if(opt == 'a'){
				printf("\nInsert into %s %s values(",table,col_names);
			}else{
				printf("\nInsert into %s %s values('%i',",table,col_names,rings);
			}
		}
	}

	rings++;
	Insert_attributes(hDBFHandle,rings-1);

	if (dump_format){
		printf("\tSRID=%s ;MULTIPOLYGON(",sr_id );
	}else{
		printf(",GeometryFromText('MULTIPOLYGON(");
	}

	for(u=0; u < out_index; u++){
		Poly = Outer[u];
		if(u==0){
			printf("(");
		}else{
			printf(",(");
		}
		if(obj->nSHPType == 5){
			while(Poly != NULL){
				for(i=0;i<Poly->n;i++){
					if(i==0){
						if(Poly != Outer[u]){
							printf(",");
						}
						printf("(%.15g %.15g ",Poly->list[i].x,Poly->list[i].y);
					}else{
						printf(",%.15g %.15g ",Poly->list[i].x,Poly->list[i].y);
					}
				}
				printf(")");
				Poly = Poly->next;
			}
			printf(")");
		}else{
  			while(Poly != NULL){
				for(i=0;i<Poly->n;i++){
					if(i==0){
						if(Poly != Outer[u]){
							printf(",");
						}
						printf("(%.15g %.15g %.15g ",Poly->list[i].x,Poly->list[i].y,Poly->list[i].z);
					}else{
						printf(",%.15g %.15g %.15g ",Poly->list[i].x,Poly->list[i].y,Poly->list[i].z);
					}
				}
				printf(")");
				Poly = Poly->next;
			}
			printf(")");
		}
	}
	if (dump_format){
		printf(")\n");
	}else{
		printf(")',%s) );",sr_id);
	}

	for(u=0; u < out_index; u++){
		Poly = Outer[u];
		while(Poly != NULL){
			temp = Poly;
			Poly = Poly->next;
			free(temp->list);
			free(temp);
		}
	}
	free(Outer);
	free(Inner);
	free(Poly);
	

	return rings;
}



//Insert the attributes from the correct row of dbf file

int
Insert_attributes(DBFHandle hDBFHandle, int row)
{
   int i,num_fields;
   char val[1024];

   num_fields = DBFGetFieldCount( hDBFHandle );
   for( i = 0; i < num_fields; i++ )
   {
      if(DBFIsAttributeNULL( hDBFHandle, row, i))
      {
         if(dump_format)
         {
				if(i) printf("\t");
            printf("\\N");
         }
         else
         {
				if(i) printf(",");
			   printf("NULL");
		   }
      }

      else /* Attribute NOT NULL */
      {
         switch (types[i]) 
         {
            case FTString:
               if ( -1 == snprintf(val, 1024, "%s",
                     DBFReadStringAttribute(hDBFHandle, row, i)) )
               {
                  fprintf(stderr, "Warning: field %d name trucated\n", i);
                  val[1023] = '\0';
               }
               break;
            case FTInteger:
               sprintf(val, "%d", DBFReadIntegerAttribute(hDBFHandle, row, i));
               break;
            case FTDouble:
               sprintf(val, "%f", DBFReadDoubleAttribute(hDBFHandle, row, i));
               break;
            default:
               fprintf(stderr,
                  "Error: field %d has invalid or unknown field type (%d)\n",
                  i, types[i]);
               exit(1);
         }
 
			if (dump_format) {
            if ( i ) printf("\t");
				printf("%s",make_good_string(val));
			} else {
            if ( i ) printf(",");
				printf("'%s'",protect_quotes_string(val));
			}
		 }
	}
	return 1;
}




// main()     
//see description at the top of this file

int main (int ARGC, char **ARGV){
	SHPHandle  hSHPHandle;
	DBFHandle  hDBFHandle;
	int num_fields,num_records,begin,trans,field_precision, field_width;
	int num_entities, phnshapetype,next_ring,errflg,c;
	double padminbound[8], padmaxbound[8];
	int u,j,z,tot_rings,curindex;
	SHPObject	*obj=NULL;
	char  name[32];
	char  *sr_id,*shp_file,*table;
	char **names;
	DBFFieldType type = -1;
	extern char *optarg;
	extern int optind;
	opt = ' ';
	errflg =0;
	j=0;
	sr_id =shp_file = table = NULL;

	while ((c = getopt(ARGC, ARGV, "kcdaDs:")) != EOF){
               switch (c) {
               case 'c':
                    if (opt == ' ')
                         opt ='c';
                    else
                         errflg =1;;
                    break;
               case 'd':
                    if (opt == ' ')
                         opt ='d';
                    else
                         errflg =1;;
                    break;
	       case 'a':
                    if (opt == ' ')
                         opt ='a';
                    else
                         errflg =1;;
                    break;
	       case 'D':
		    dump_format =1;
                    break;
               case 's':
                    sr_id = optarg;
                    break;
               case 'k':
                    quoteidentifiers = 1;
                    break;
               case '?':
                    errflg=1;
               }
	}

	if(sr_id == NULL){
		sr_id = "-1";
	}

	if(opt == ' '){
		opt = 'c';
	}

	curindex=0;
        for ( ; optind < ARGC; optind++){
		if(curindex ==0){
			shp_file = ARGV[optind];
		}else if(curindex == 1){
			table = ARGV[optind];
		}
		curindex++;
	}
	
	/*
	 * Third argument (if present) is supported for compatibility
	 * with old shp2pgsql versions taking also database name.
	 */
	if(curindex < 2 || curindex > 3){
		errflg = 1;
	}

        if (errflg==1) {
		printf("\n**ERROR** invalid option or command parameters\n");
		printf("\n");
		printf("USAGE: shp2pgsql [<options>] <shapefile name> <table name>\n");
		printf("\n");
		printf("OPTIONS:\n");
		printf("  -s <srid>  Set the SRID field. If not specified it defaults to -1.\n");
		printf("\n");
		printf("  (-d|a|c) These are mutually exclusive options:\n");
		printf("      -d  Drops the table , then recreates it and populates\n");
		printf("          it with current shape file data.\n");
		printf("      -a  Appends shape file into current table, must be\n");
		printf("          exactly the same table schema.\n");
		printf("      -c  Creates a new table and populates it, this is the\n");
		printf("          default if you do not specify any options.\n");
		printf("\n");
		printf("  -D  Use postgresql dump format (defaults to sql insert\n");
		printf("      statments.\n");
		printf("\n");
		printf("  -k  Keep postgresql identifiers case.\n");
		exit (2);
        }


	//Open the shp and dbf files
	hSHPHandle = SHPOpen( shp_file, "rb" );
	hDBFHandle = DBFOpen( shp_file, "rb" );
	if (hSHPHandle == NULL || hDBFHandle == NULL){
		fprintf(stderr, "shape is null\n");
		exit(-1);
	}

	if(opt == 'd')
	{
		//-------------------------Drop the table--------------------------------
		printf("select DropGeometryColumn('','%s','the_geom');", table);
		if ( quoteidentifiers ){
			printf("\ndrop table \"%s\";\n",table);
		}else{
			printf("\ndrop table %s;\n",table);
		}
	}


	/*
	 * Get col names and types to fully specify them in inserts
	 */
	num_fields = DBFGetFieldCount( hDBFHandle );
	num_records = DBFGetRecordCount(hDBFHandle);
	names = malloc(num_fields*sizeof(char*));
	types = (DBFFieldType *)malloc(num_fields*sizeof(char*));
	widths = malloc(num_fields*sizeof(int));
	precisions = malloc(num_fields*sizeof(int));
	col_names = malloc(num_fields * sizeof(char) * 32);
	if(opt != 'a'){
		strcpy(col_names, "(gid," );
	}else{
		strcpy(col_names, "(" );
	}

	for(j=0;j<num_fields;j++){
		type = DBFGetFieldInfo(hDBFHandle, j, name, &field_width, &field_precision); 

//fprintf(stderr, "Field %d (%s) width/decimals: %d/%d\n", j, name, field_width, field_precision);
		names[j] = malloc ( strlen(name)+3 );
		types[j] = type;
		widths[j] = field_width;
		precisions[j] = field_precision;
		strcpy(names[j], name);
		for(z=0; z < j ; z++){
			if(strcmp(names[z],name)==0){
				strcat(name,"__");
				sprintf(name,"%s%i",name,j);
				break;
			}
		}	
		if(strcasecmp(name,"gid")==0){
			if(j ==0){
				sprintf(col_names,"%s%s__2",col_names,name);
			}else{
				sprintf(col_names,"%s,%s__2",col_names,name);
			}
		}else{
			if ( quoteidentifiers ){
				if(j ==0){
					sprintf(col_names,"%s\"%s\"",col_names,name);
				}else{
					sprintf(col_names,"%s,\"%s\"",col_names,name);
				}
			}else{
				if(j ==0){
					sprintf(col_names,"%s%s",col_names,name);
				}else{
					sprintf(col_names,"%s,%s",col_names,name);
				}
			}
		}
	}
	strcat(col_names, ",the_geom)");


	//if opt is 'a' do nothing, go straight to making inserts
	if(opt == 'c' || opt == 'd')
	{

		/* 
		 * Create a table for inserting the shapes into with appropriate
		 * columns and types
		 */

		if ( quoteidentifiers ){
			printf("CREATE TABLE \"%s\" (gid serial", table);
		}else{
			printf("CREATE TABLE %s (gid serial", table);
		}

		for(j=0;j<num_fields;j++)
		{
			type = types[j];
			field_width = widths[j];
			field_precision = precisions[j];


			if ( quoteidentifiers ) printf(", \"%s\" ", names[j]);
			else printf(", %s ", names[j]);

			if(hDBFHandle->pachFieldType[j] == 'D' ) /* Date field */
			{
				printf ("varchar(8)");//date data-type is not supported in API so check for it explicity before the api call.
			}
			
			else
			{

				if(type == FTString){
					printf ("varchar");
				}else if(type == FTInteger){
					if( field_width > 18 ){
						printf ("numeric(%d,0)",field_width);
					}else if( field_width > 9){
						printf ("int8");
					}else{
						printf ("int4");
					}
				}else if(type == FTDouble){
					if( field_width > 18 ){
						//printf ("numeric(%d,%d)",field_width, field_precision);
						printf ("numeric");
					}else{
						printf ("float8");
					}
				}else{
					printf ("Invalid type in DBF file");
				}
			}	
		}
		printf (");\n");
		//finished creating the table
	}



	SHPGetInfo( hSHPHandle, &num_entities, &phnshapetype, &padminbound[0], &padmaxbound[0]);
	j=num_entities;
	while(obj == NULL && j--)
	{
		obj = SHPReadObject(hSHPHandle,j);
	}
	if ( obj == NULL) 
	{
		fprintf(stderr, "Shapefile contains %d NULL object(s)\n", num_entities);
		exit(-1);
	}

	trans=0;
	

	//create the geometry column with an addgeometry call to dave's function
	if(opt != 'a'){
		printf("select AddGeometryColumn('','%s','the_geom','%s',",table,sr_id);
		if( obj->nSHPType == 1 ){  //2d point
			printf("'POINT',2);\n");
		}else if( obj->nSHPType == 21){ // PointM
			printf("'POINT',2);\n");
		}else if( obj->nSHPType == 23){  // PolyLineM
			printf("'MULTILINESTRING',2);\n");
		}else if( obj->nSHPType == 25){  // PolygonM
			printf("'MULTIPOLYGON',2);\n");
		}else if( obj->nSHPType == 28){ // MultiPointM
		         printf("'MULTIPOINT',2);\n");
		}else if( obj->nSHPType == 3){	//2d arcs/lines
			printf("'MULTILINESTRING',2);\n");
		}else if( obj->nSHPType == 5){	//2d polygons
			printf("'MULTIPOLYGON',2);\n");
		}else if( obj->nSHPType == 8){ //2d multipoint
		         printf("'MULTIPOINT',2);\n");
		}else if( obj->nSHPType == 11){ //3d points
			printf("'POINT',3);\n");
		}else if( obj->nSHPType == 13){  //3d arcs/lines
			printf("'MULTILINESTRING',3);\n");
		}else if( obj->nSHPType == 15){  //3d polygons
			printf("'MULTIPOLYGON',3);\n");
		}else if( obj->nSHPType == 18){  //3d multipoints
			printf("'MULTIPOINT',3);\n");
		}else{
			printf("'GEOMETRY',3);\n");
		}
	}
	if (dump_format){
			printf("COPY \"%s\" %s FROM stdin;\n",table, col_names);
	}
	

	if (obj->nVertices == 0){
		if (dump_format){
			printf("\\N");
			Insert_attributes(hDBFHandle,j);//add the attributes of each shape to the insert statement
			SHPDestroyObject(obj);
		}else{
			printf("NULL");
			Insert_attributes(hDBFHandle,j);//add the attributes of each shape to the insert statement
			SHPDestroyObject(obj);	
		}
	}
	else 
	{


	   //Determine what type of shape is in the file and do appropriate processing

	   if(( obj->nSHPType == 5 ) || ( obj->nSHPType == 25 )){
           //---------------------------------------------------------------------------------
           //---------POLYGONS----------------------------------------------------------------

		// sorts of all the rings so that they are outer,inner,iner,outer,inner...
		// with the inner ones coming after the outer ones they are within spatially

		tot_rings = 0;


		//go through each entity and call ring_check() to sort the rings and print out the sql statement
		// keep track of total number of inserts in tot_rings so
		// you can pass it to the function for the next entity
		for (j=0;j<num_entities; j++){

			//wrap a transaction block around each 250 inserts...
			if(trans == 250 || j==0){
				if(j==0){
					if (!(dump_format) )
						printf("begin;");
				}else{
					if (!(dump_format) ){
						printf("end;\n");
						printf("begin;");
					}
				}
				trans=0;
			}
			trans++;
			// transaction stuff done

			obj = SHPReadObject(hSHPHandle,j);	//open the next object


			tot_rings = ring_check(obj,table,sr_id,tot_rings,hDBFHandle);
			SHPDestroyObject(obj); //close the object
		}

		if (!(dump_format) ){
			printf("end;");	//End the last transaction block
		}

	}else if( obj->nSHPType == 1 || obj->nSHPType == 21 ){
		//---------------------------------------------------------------------
		//----------POINTS-----------------------------------------------------

		for (j=0;j<num_entities; j++){

			//wrap a transaction block around each 250 inserts...
			if(trans == 250 || j==0){
				if(j==0){
					if (!(dump_format) )
						printf("begin;");
				}else{
					if (!(dump_format) ){
						printf("end;\n");
						printf("begin;");
					}
				}
				trans=0;
			}
			trans++;
			// transaction stuff done

			if (dump_format){
				if(opt != 'a'){
					printf("%i\t",j);
				}
			}else{
				if ( quoteidentifiers ){
					if(opt == 'a'){
						printf("insert into \"%s\" %s values (",table,col_names);
					}else{
						printf("insert into \"%s\" %s values ('%i',",table,col_names,j);
					}
				}else{
					if(opt == 'a'){
						printf("insert into %s %s values (",table,col_names);
					}else{
						printf("insert into %s %s values ('%i',",table,col_names,j);
					}
				}
			}

			Insert_attributes(hDBFHandle,j); //add the attributes for each entity to the insert statement



			if (dump_format){
				printf("\tSRID=%s;POINT(",sr_id);
			}else{
				printf(",GeometryFromText('POINT (");
			}
			obj = SHPReadObject(hSHPHandle,j);

			for (u=0;u<obj->nVertices; u++){
				if (u>0){
					printf(",%.15g %.15g",obj->padfX[u],obj->padfY[u]);
				}else{
					printf("%.15g %.15g",obj->padfX[u],obj->padfY[u]);
				}
			}
			if (dump_format){
				printf(")\n");

			}
			else{
				printf(")',%s) );\n",sr_id);
			}

			SHPDestroyObject(obj);



		}
		if (!(dump_format) ){
			printf("end;"); //End the last transaction
		}
	}else if( obj->nSHPType == 3 || obj->nSHPType == 23 ){
		//------------------------------------------------------------------------
		//--------ARCs / LINES----------------------------------------------------

		begin=0;//initialize the begin flag

		for (j=0;j<num_entities; j++){
		
			//wrap a transaction around each 250 inserts...
			if(trans == 250 || j==0){
				if(j==0){
					if (!(dump_format) )
						printf("begin;\n");
				}else{
					if (!(dump_format) )
						printf("end;\n");
					if (!(dump_format) )
						printf("begin;");
				}
				trans=0;
			}
			trans++;
			//end of transaction stuff

			if (dump_format){
				if(opt != 'a'){				
					printf("%i\t",j);
				}
			}else{
				if ( quoteidentifiers ){
					if(opt == 'a'){
						printf("insert into \"%s\" %s values(",table,col_names);
					}else{
						printf("insert into \"%s\" %s values('%i',",table,col_names,j);
					}
				}else{
					if(opt == 'a'){
						printf("insert into %s %s values(",table,col_names);
					}else{
						printf("insert into %s %s values('%i',",table,col_names,j);
					}
				}
			}
			obj = SHPReadObject(hSHPHandle,j);

			if(obj->nParts >1){
				next_ring = 1;//if there is more than one part, set the next_ring index to one
			}else{
				next_ring = -99;
			}
			

			Insert_attributes(hDBFHandle,j); //add the attributes of each shape to the insert statement

			/* Invalid (MULTI)Linestring */
			if ( obj->nVertices < 2 )
			{
				fprintf(stderr,
	"MULTILINESTRING %d as %d vertices, set to NULL\n",
	j, obj->nVertices);
				if (dump_format){
		               		printf("\t\\N\n");
				}else{
					printf(",NULL);\n");
				}

				SHPDestroyObject(obj);

				continue;
			}

			if (dump_format){
				printf("\tSRID=%s;MULTILINESTRING(",sr_id);
			}else{
				printf(",GeometryFromText('MULTILINESTRING (");
			}

			//for each vertice write out the coordinates in the insert statement, when there is a new line 
			//you must end the brackets and start new ones etc.
			for (u=0;u<obj->nVertices; u++){
				
				//check if the next vertice is the start of a new line
		                //printf("\n\nu+1 = %d, next_ring = %d  index = %d\n",u+1,next_ring,obj->panPartStart[next_ring]);

				if(next_ring==-99 && obj->nVertices ==1){	
					printf("(%.15g %.15g )",obj->padfX[u],obj->padfY[u]);
				}else if((next_ring != -99)&& (begin==1) && (u+1 == obj->panPartStart[next_ring]) ){
					printf("(%.15g %.15g )",obj->padfX[u],obj->padfY[u]);
					next_ring++;
					begin=1;
				}else if(((next_ring != -99) && (u+1 == obj->panPartStart[next_ring] )) || u==(obj->nVertices-1) ){
					printf(",%.15g %.15g ",obj->padfX[u],obj->padfY[u]);
					printf(")");
					
					next_ring++;
					begin=1;//flag the fact that you area at a new line next time through the loop
				}else{
					if (u==0 || begin==1){ //if you are at the begging of a new line add comma and brackets 
						if(u!=0) printf(",");
						printf("(%.15g %.15g ",obj->padfX[u],obj->padfY[u]);
						begin=0;
					}else{
						printf(",%.15g %.15g ",obj->padfX[u],obj->padfY[u]);
					}
				}
			}	
			
			if (dump_format)
			{
				printf(")\n");
			}
			else
				printf(")',%s) );\n",sr_id);

			
			SHPDestroyObject(obj);
		}

		if (!(dump_format) )
			printf("end;");//end the last transaction
		

	}else if( obj->nSHPType == 8 || obj->nSHPType == 28 ){
		//---------------------------------------------------------------------
		//----------MULTIPOINTS------------------------------------------------

		for (j=0;j<num_entities; j++){
			
			//wrap a transaction block around each 250 inserts...
			if(trans == 250 || j==0){
				if(j==0){
					if (!(dump_format) )
						printf("begin;");
				}else{
					if (!(dump_format) ){
						printf("end;\n");
						printf("begin;");
					}
				}
				trans=0;
			}
			trans++;
			// transaction stuff done

			if (dump_format){
				if(opt != 'a'){
					printf("%i\t",j);
				}
			}else{			
				if ( quoteidentifiers ){
					if(opt == 'a'){
						printf("insert into \"%s\" %s values (",table,col_names);
					}else{
						printf("insert into \"%s\" %s values ('%i',",table,col_names,j);
					}
				}else{
					if(opt == 'a'){
						printf("insert into %s %s values (",table,col_names);
					}else{
						printf("insert into %s %s values ('%i',",table,col_names,j);
					}
				}
			}

			Insert_attributes(hDBFHandle,j); //add the attributes for each entity to the insert statement



			if (dump_format){
				printf("\tSRID=%s;MULTIPOINT(",sr_id);
			}else{
				printf(",GeometryFromText('MULTIPOINT ("); 
			}
			obj = SHPReadObject(hSHPHandle,j);
			
			for (u=0;u<obj->nVertices; u++){
				if (u>0){
					printf(",%.15g %.15g",obj->padfX[u],obj->padfY[u]);
				}else{
					printf("%.15g %.15g",obj->padfX[u],obj->padfY[u]);
				}
			}
			if (dump_format){
				printf(")\n");

			}
			else{
				printf(")',%s) );\n",sr_id);
			}
			
			SHPDestroyObject(obj);



		}
		if (!(dump_format) ){
			printf("end;"); //End the last transaction
		}
	}else if( obj->nSHPType == 11 ){  
		//---------------------------------------------------------------------
		//----------POINTZ-----------------------------------------------------

		for (j=0;j<num_entities; j++){
			
			//wrap a transaction block around each 250 inserts...
			if(trans == 250 || j==0){
				if(j==0){
					if (!(dump_format) )
						printf("begin;");
				}else{
					if (!(dump_format) ){
						printf("end;\n");
						printf("begin;");
					}
				}
				trans=0;
			}
			trans++;
			// transaction stuff done

			if (dump_format){
				if(opt != 'a'){
					printf("%i\t",j);
				}
			}else{			
				if ( quoteidentifiers ){
					if(opt == 'a'){
						printf("insert into \"%s\" %s values (",table,col_names);
					}else{
						printf("insert into \"%s\" %s values ('%i',",table,col_names,j);
					}
				}else{
					if(opt == 'a'){
						printf("insert into %s %s values (",table,col_names);
					}else{
						printf("insert into %s %s values ('%i',",table,col_names,j);
					}
				}
			}

			Insert_attributes(hDBFHandle,j); //add the attributes for each entity to the insert statement



			if (dump_format){
				printf("\tSRID=%s;POINT(",sr_id);
			}else{
				printf(",GeometryFromText('POINT ("); 
			}
			obj = SHPReadObject(hSHPHandle,j);
			
			for (u=0;u<obj->nVertices; u++){
				if (u>0){
					printf(",%.15g %.15g %.15g",obj->padfX[u],obj->padfY[u],obj->padfZ[u]);
				}else{
					printf("%.15g %.15g %.15g",obj->padfX[u],obj->padfY[u],obj->padfZ[u]);
				}
			}
			if (dump_format){
				printf(")\n");

			}
			else{
				printf(")',%s) );\n",sr_id);
			}
			
			SHPDestroyObject(obj);



		}
		if (!(dump_format) ){
			printf("end;"); //End the last transaction
		}
	}else if( obj->nSHPType == 13 ){  
		//---------------------------------------------------------------------
		//------Linez(3D lines)--------------------------------------------
		
		begin=0;//initialize the begin flag
				
		for (j=0;j<num_entities; j++){

			//wrap a transaction around each 250 inserts...
			if(trans == 250 || j==0){
				if(j==0){
					if (!(dump_format) )
						printf("begin;");
				}else{
					if (!(dump_format) )
						printf("end;\n");
					if (!(dump_format) )
						printf("begin;");
				}
				trans=0;
			}
			trans++;
			//end transaction stuff

			if (dump_format){
				if(opt != 'a'){
					printf("%i\t",j);
				}
			}else{
				if ( quoteidentifiers ){
					if(opt == 'a'){
						printf("insert into \"%s\" %s values (",table,col_names);
					}else{
						printf("insert into \"%s\" %s values ('%i',",table,col_names,j);
					}
				}else{
					if(opt == 'a'){
						printf("insert into %s %s values (",table,col_names);
					}else{
						printf("insert into %s %s values ('%i',",table,col_names,j);
					}
				}
			}

			obj = SHPReadObject(hSHPHandle,j);

			if(obj->nParts >1){
				next_ring = 1;//if there is more than one part, set the next_ring index to one
			}else{
				next_ring = -99;
			}

			Insert_attributes(hDBFHandle,j);//add the attributes of each shape to the insert statement

			/* Invalid (MULTI)Linestring */
			if ( obj->nVertices < 2 )
			{
				fprintf(stderr,
	"MULTILINESTRING %d as %d vertices, set to NULL\n",
	j, obj->nVertices);
				if (dump_format){
		               		printf("\t\\N\n");
				}else{
					printf(",NULL);\n");
				}

				SHPDestroyObject(obj);

				continue;
			}


			if (dump_format){
				printf("\tSRID=%s;MULTILINESTRING(",sr_id);
			}else{
				printf(",GeometryFromText('MULTILINESTRING (");
			}

			//for each vertice write out the coordinates in the insert statement, when there is a new line 
			//you must end the brackets and start new ones etc.
			for (u=0;u<obj->nVertices; u++){

				//check if the next vertice is the start of a new line				
				if(((next_ring != -99) && (u+1 == obj->panPartStart[next_ring] )) || u==(obj->nVertices-1) ){
					printf(",%.15g %.15g %.15g ",obj->padfX[u],obj->padfY[u],obj->padfZ[u]);
					printf(")");
					next_ring++;
					begin =1;//flag the fact that you area at a new line next time through the loop
				}else{
					if (u==0 || begin==1){
						if(u!=0) printf(",");
						printf("(%.15g %.15g %.15g ",obj->padfX[u],obj->padfY[u],obj->padfZ[u]);
						begin=0;
					}else{
						printf(",%.15g %.15g %.15g ",obj->padfX[u],obj->padfY[u],obj->padfZ[u]);
					}
				}
			}	
			
			if (dump_format){
				printf(")\n");
			}else{
				printf(")',%s));\n",sr_id);
			}

			
			SHPDestroyObject(obj);
		}

		if (!(dump_format) )
			printf("end;");//close last transaction
	
       }else if( obj->nSHPType == 15 ){
           //---------------------------------------------------------------------------------
           //---------POLYGONZ (3D POLYGONS)--------------------------------------------------

		// sorts of all the rings so that they are outer,inner,iner,outer,inner...
		// with the inner ones coming after the outer ones they are within spatially

		tot_rings = 0;


		//go through each entity and call ring_check() to sort the rings and print out the sql statement
		// keep track of total number of inserts in tot_rings so
		// you can pass it to the function for the next entity
		for (j=0;j<num_entities; j++){

			//wrap a transaction block around each 250 inserts...
			if(trans == 250 || j==0){
				if(j==0){
					if (!(dump_format) )
						printf("begin;");
				}else{
					if (!(dump_format) ){
						printf("end;\n");
						printf("begin;");
					}
				}
				trans=0;
			}
			trans++;
			// transaction stuff done

			obj = SHPReadObject(hSHPHandle,j);	//open the next object


			tot_rings = ring_check(obj,table,sr_id,tot_rings,hDBFHandle);
			SHPDestroyObject(obj); //close the object
		}

		if (!(dump_format) ){
			printf("end;");	//End the last transaction block
		}


	}else if( obj->nSHPType == 18 ){
		//---------------------------------------------------------------------------
		//------MULTIPOINTZ (3D MULTIPOINTS)-----------------------------------------

		for (j=0;j<num_entities; j++){

			//wrap a transaction around each 250 inserts...
			if(trans == 250 || j==0){
				if(j==0){
					if (!(dump_format) )
						printf("begin;\n");
				}else{
					if (!(dump_format) )
						printf("end;\n");
					if (!(dump_format) )
						printf("begin;\n");
				}
				trans=0;
			}
			trans++;
			//end of transaction stuff

			if (dump_format){
				if(opt != 'a'){
					printf("%i\t",j);
				}
			}else{
				if ( quoteidentifiers ){
					if(opt == 'a'){
						printf("insert into \"%s\" %s values(",table,col_names);
					}else{
						printf("insert into \"%s\" %s values('%i',",table,col_names,j);
					}
				}else{
					if(opt == 'a'){
						printf("insert into %s %s values(",table,col_names);
					}else{
						printf("insert into %s %s values('%i',",table,col_names,j);
					}
				}
			}

			obj = SHPReadObject(hSHPHandle,j);

			Insert_attributes(hDBFHandle,j);//add the attributes of each shape to the insert statement


			if (dump_format){
				printf("\tSRID=%s;MULTIPOINT(",sr_id);
			}else{
				printf(",GeometryFromText('MULTIPOINT (");
			}

			for (u=0;u<obj->nVertices; u++){
				if (u>0){
					printf(",%.15g %.15g %.15g",obj->padfX[u],obj->padfY[u],obj->padfZ[u]);
				}else{
					printf("%.15g %.15g %.15g",obj->padfX[u],obj->padfY[u],obj->padfZ[u]);
				}
			}

			if (dump_format)
			{
				printf(")\n");
			}
			else
				printf(")',%s));\n",sr_id);


			SHPDestroyObject(obj);
		}


		if (!(dump_format) )
			printf("end;");//end the last transaction
	}else{
		printf ("\n\n**** Type is NOT SUPPORTED, type id = %d ****\n\n",obj->nSHPType);
		//print out what type the file is and that it is not supported

	}//end the if statement for shape types

	}

	if ((dump_format) ) {
		printf("\\.\n");

	}
	free(col_names);
	if(opt != 'a'){
		printf("\nALTER TABLE ONLY %s ADD CONSTRAINT %s_pkey PRIMARY KEY (gid);\n",table,table);
		if(j > 1){
			printf("SELECT setval ('%s_gid_seq', %i, true);\n", table, j-1);
		}
	}

	return(1);
}//end main()
