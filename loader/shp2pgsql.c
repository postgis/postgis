//compile line for Solaris
//gcc -g pop.c ../shapelib-1.2.8/shpopen.o ../shapelib-1.2.8/dbfopen.o -o pop


//  usage:   pop  <shapefile to process>  <table name> <SR_ID> [-d || -a || -c] [-dump] | psql -h <host> -d <database> -p <port> ...
// -d: drops the table , then recreates it and populates it with current shape file data
// -a: appends shape file into current table, must be excatly the same table schema
// -c: creates a new table and populates it, this is the default if you don't specify any options

// -dump : use postgresql dump format (defaults to sql insert statements)

//	Using shapelib 1.2.8, this program reads in shape files and processes it's contents
//	into a Insert statements which can be easily piped into a database frontend.
//	Specifically designed to insert type 'geometry' (a custom written PostgreSQL type)
//	for the shape files and PostgreSQL standard types for all attributes of the entity. 
//
//	Basically the program determines which type of shape (currently supports: 2d points,2d lines,2d
//      polygons,3d points, and 3d lines) is in the file and takes appropriate
//      action to read out the attributes.


// BUGS:
//	possible: no row # for polygons?

#include "shapefil.h"
#include <stdio.h>
#include <string.h>

typedef struct {double x, y;} Point;

typedef struct Ring{
	Point *list;	//list of points
	struct Ring  *next;
	int		n;		//number of points in list
} Ring;

int	dump_format = 0; //0=insert statements, 1 = dump

int Insert_attributes(DBFHandle hDBFHandle, char **ARGV, int row);


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

	str2 =  str;

	while (str2 = strchr(str2, '\t') )
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

	while(end = strchr(start,'\t'))
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

char	*protect_quotes_string(char *str)
{
	//find all the tabs and make them \<tab>s
	// 
	// 1. find # of quotes
	// 2. make new string 

	char	*result;
	char	*str2;
	char	*start,*end;
	int	num_tabs = 0;

	str2 =  str;

	while (str2 = strchr(str2, '\'') )
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

	while(end = strchr(start,'\''))
	{
		if ( (end == str) || (end[-1] != '\\' ) ) //the previous char isnt a '\'
		{

			strncat(result, start, (end-start));	
			strcat(result,"\\'");
			start = end +1;
		}
		else
		{
			strncat(result, start, (end-start));	
			strcat(result,"'");	
			start = end  +1;
		}

	}
	strcat(result,start);
	return result;
}



// PIP(): crossing number test for a point in a polygon
//      input:   P = a point,
//               V[] = vertex points of a polygon V[n+1] with V[n]=V[0]
//      returns: 0 = outside, 1 = inside
int PIP( Point P, Point* V, int n )
{
    int cn = 0;    // the crossing number counter
	int i;
    // loop through all edges of the polygon
    for (i=0; i<n; i++) {    // edge from V[i] to V[i+1]
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

int ring_check(SHPObject* obj, char **ARGV, int rings, DBFHandle hDBFHandle){
	Point pt,pt2;
	Ring *Poly;
	Ring *temp;
	Ring **Outer; //pointer to a list of outer polygons
	Ring **Inner; //pointer to a list of inner polygons
	int out_index,in_index,indet_index; //indexes to the lists **Outer and **Inner
	int	in,temp2;
	int u,i,N,n,new_outer;
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
		if(((next_ring != -99) && (u+1 == obj->panPartStart[next_ring] )) || u==N-1){
			//check if a ring is clockwise(outer) or not(inner) by getting positive(inner) or negative(outer) area.
			//'area' is actually twice actual polygon area so divide by 2, not that it matters but in case we use it latter...
			area = area/2.0;
			if(area < 0.0 || obj->nParts ==1){
				
				//An outer ring so fill in the last point then put it in the 'Outer' list
				Poly->list[n].x = obj->padfX[u]; //the polygon is ended with it's first co-ordinates reused
				Poly->list[n].y = obj->padfY[u];
				Poly->n = n+1;
				Outer[out_index] = Poly;
				out_index++;
				
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
				
				n=0;//set your count of what point you are at in the current ring back to 0
		
			}else{
				
				Poly->list[n].x = obj->padfX[u]; //the polygon ends with it's first co-ordinates reused
				Poly->list[n].y = obj->padfY[u];
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
			if(in==1 && PIP(pt2,Outer[i]->list,Outer[i]->n)){
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
		printf("%i",rings);
	}else{
		printf("\nInsert into %s values('%i'",ARGV[2],rings);
	}

	rings++;
	Insert_attributes(hDBFHandle, ARGV,rings-1);

	if (dump_format){
		printf("%i\tSRID=%s ;MULTIPOLYGON(",rings,ARGV[3] );
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
			temp = Poly;
			Poly = Poly->next;
			free(temp->list);
			free(temp);
		}
		printf(")");
	}
	if (dump_format){
		printf(")\n"); 
	}else{
		printf(")',%s) );",ARGV[3]);
	}

	free(Outer);
	free(Inner);
	free(Poly);
	

	return rings;
}



//Insert the attributes from the correct row of dbf file

int Insert_attributes(DBFHandle hDBFHandle, char **ARGV, int row){

	int i,num_fields;


	num_fields = DBFGetFieldCount( hDBFHandle );
		
	
	for( i = 0; i < num_fields; i++ ){
			if (dump_format){
				printf("\t%s",make_good_string(DBFReadStringAttribute( hDBFHandle, row, i )) );
			}else{
				printf(",'%s'",protect_quotes_string(DBFReadStringAttribute( hDBFHandle, row, i )) );
			}
	}
	return 1;
}




// main()     
//  usage:   pop  <shapefile to process> <table name> <SR_ID> <database name> [-d || -a || -c] [-dump]| psql -h <host> -d <database> -p <port> ...
// -d: drops the table , then recreates it and populates it with current shape file data
// -a: appends shape file into current table, must be excatly the same table schema
// -c: creates a new table and populates it, this is the default if you don't specify any options

// -dump : use postgresql dump format (defaults to sql insert statements)

//	Using shapelib 1.2.8, this program reads in shape files and processes it's contents
//	into a Insert statements which can be easily piped into a database frontend.
//	Specifically designed to insert type 'geometry' (a custom written PostgreSQL type)
//	for the shape files and PostgreSQL standard types for all attributes of the entity. 
//
//	Basically the program determines which type of shape (currently supports: 2d points,2d lines,2d
//  polygons,3d points, and 3d lines) is in the file and takes appropriate action to read out the attributes.


main (int ARGC, char **ARGV)
{
    SHPHandle   hSHPHandle;
	DBFHandle	hDBFHandle;
	int num_fields,num_records,begin,trans;
	int num_entities,  phnshapetype,next_ring;
	double padminbound[8], padmaxbound[8];
	int u,j,tot_rings;
	SHPObject	*obj;
	char		name[12];
	char		opt;
	DBFFieldType type;

	//display proper usage if incorrect number of arguments given	
	if (ARGC != 5 && ARGC != 6 && ARGC != 7)
	{
		printf(" usage:   pop  <shapefile to process>  <table name> <SR_ID> <database> [-d || -a || -c] [-dump]| psql -h <host> -d <database> -p <port> ...\n");
		printf(" -d: drops the table , then recreates it and populates it with current shape file data\n");
		printf(" -a: appends shape file into current table, must be excatly the same table schema\n");
		printf(" -c: creates a new table and populates it, this is the default if you don't specify any options\n");
		printf(" -dump : use postgresql dump format (defaults to sql insert statements)\n");

		exit (-1);
	}

	
	opt = 'c';

	if(ARGC ==6){
		if(strcmp(ARGV[5], "-d")==0){
			opt = 'd';
		}else if(strcmp(ARGV[5], "-c")==0){
			opt = 'c';
		}else if(strcmp(ARGV[5], "-a")==0){
			opt = 'a';
		}else if (strcmp(ARGV[5],"-dump") == 0) {
			dump_format = 1;
		}else{
			printf("option %s is not a valid option, use -a, -d, or -c\n",ARGV[5]);
		exit(-1);
		}
	}else{
		opt = 'c';
	}

	if (ARGC==7)
	{
		if(strcmp(ARGV[5], "-d")==0){
			opt = 'd';
		}else if(strcmp(ARGV[5], "-c")==0){
			opt = 'c';
		}else if(strcmp(ARGV[5], "-a")==0){
			opt = 'a';
		}else{
			printf("option(5) %s is not a valid option, use -a, -d, or -c\n",ARGV[5]);
			exit(-1);
		}




		if (strcmp(ARGV[6],"-dump") == 0) 
		{
			dump_format = 1;
		}else{
			printf("option(6) %s is not a valid option, use -dump \n",ARGV[6]);
			exit(-1);
		}

	}

	
	//Open the shp and dbf files
	hSHPHandle = SHPOpen( ARGV[1], "rb" );
	hDBFHandle = DBFOpen( ARGV[1], "rb" );
	if (hSHPHandle == NULL || hDBFHandle == NULL){
		printf ("shape is null\n");	
		exit(-1);
	}

	if(opt == 'd'){
		//-------------------------Drop the table--------------------------------
		//drop the table given
		printf("select DropGeometryColumn('%s','%s','the_geom');",ARGV[4],ARGV[2]);
		printf("\ndrop table %s;\n",ARGV[2]);


	}

	

	if(opt == 'c' || opt == 'd'){ //if opt is 'a' do nothing, go straight to making inserts

		//-------------------------Create the table--------------------------------
		//create a table for inserting the shapes into with appropriate columns and types

		printf("create table %s (gid int4 ",ARGV[2]);

		num_fields = DBFGetFieldCount( hDBFHandle );
		num_records = DBFGetRecordCount(hDBFHandle);

		for(j=0;j<num_fields;j++){
			type = DBFGetFieldInfo(hDBFHandle, j, name, NULL, NULL);
			
			if(strcmp(name,"gid")==0){
				printf(", %s__2 ",name);
			}else{
				printf(", %s ",name);
			}
	
			if(hDBFHandle->pachFieldType[j] == 'D' ){
				printf ("varchar(8)");//date data-type is not supported in API so check for it explicity before the api call.
			}else{
				if(type == FTString){
					printf ("varchar");
				}else if(type == FTInteger){
					printf ("int4");			
				}else if(type == FTDouble){
					printf ("float8");
				}else{
					printf ("Invalid type in DBF file");
				}
			}	
		}
		printf (");\n");
		//finished creating the table
		


	}


	SHPGetInfo( hSHPHandle, &num_entities, &phnshapetype, &padminbound[0], &padmaxbound[0]);
	obj = 	SHPReadObject(hSHPHandle,0);
	trans=0;


	//create the geometry column with an addgeometry call to dave function


	printf("select AddGeometryColumn('%s','%s','the_geom',%s,",ARGV[4],ARGV[2],ARGV[3]);

	if( obj->nSHPType == 1 ){  //2d point
		printf("'MULTIPOINT',2);\n");
	}else if( obj->nSHPType == 3){	//2d arcs/lines
		printf("'MULTILINESTRING',2);\n");
	}else if( obj->nSHPType == 5){	//2d polygons
		printf("'MULTIPOLYGON',2);\n");
	}else if( obj->nSHPType == 11){ //3d points
		printf("'MULTIPOINT',3);\n");
	}else if( obj->nSHPType == 13){  //3d arcs/lines
		printf("'MULTILINESTRING',3);\n");
	}else{
		printf("'GEOMETRY',3);\n");
	}
	
	if (dump_format){
			printf("COPY \"%s\" from stdin;\n",ARGV[2]);
	}
	
	
	//Determine what type of shape is in the file and do appropriate processing
	if (obj->nVertices == 0){
		if (dump_format){
			printf("\\N");
			Insert_attributes(hDBFHandle, ARGV,j);//add the attributes of each shape to the insert statement
			SHPDestroyObject(obj);
		}else{
			printf("NULL");
			Insert_attributes(hDBFHandle, ARGV,j);//add the attributes of each shape to the insert statement
			SHPDestroyObject(obj);	
		}
	}
	else 
	{



		if( obj->nSHPType == 5 ){  
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
		

			tot_rings = ring_check(obj,ARGV,tot_rings,hDBFHandle);
			SHPDestroyObject(obj); //close the object
		}

		if (!(dump_format) ){
			printf("end;");	//End the last transaction block
		}
	
	}else if( obj->nSHPType == 1){  
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
				printf("%i",j);
			}else{			
				printf("insert into %s values ('%i'",ARGV[2],j);
			}

			Insert_attributes(hDBFHandle, ARGV,j); //add the attributes for each entity to the insert statement



			if (dump_format){
				printf("\tSRID=%s;MULTIPOINT(",ARGV[3]);
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
				printf(")',%s) );\n",ARGV[3]);
			}
			
			SHPDestroyObject(obj);



		}
		if (!(dump_format) ){
			printf("end;"); //End the last transaction
		}
	}else if( obj->nSHPType == 3){
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
				printf("%i",j);
			}else{
				printf("insert into %s values ('%i'",ARGV[2],j);
			}
			obj = SHPReadObject(hSHPHandle,j);

			if(obj->nParts >1){
				next_ring = 1;//if there is more than one part, set the next_ring index to one
			}else{
				next_ring = -99;
			}
			

			Insert_attributes(hDBFHandle, ARGV,j); //add the attributes of each shape to the insert statement


			if (dump_format){
				printf("\tSRID=%s;MULTILINESTRING(",ARGV[3]);
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
				printf(")',%s) );\n",ARGV[3]);

			
			SHPDestroyObject(obj);
		}

		if (!(dump_format) )
			printf("end;");//end the last transaction
		

	
	}else if( obj->nSHPType == 13){  
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
				printf("%i",j);
			}else{
				printf("insert into %s values ('%i'",ARGV[2],j);
			}

			obj = SHPReadObject(hSHPHandle,j);

			if(obj->nParts >1){
				next_ring = 1;//if there is more than one part, set the next_ring index to one
			}else{
				next_ring = -99;
			}

			Insert_attributes(hDBFHandle, ARGV,j);//add the attributes of each shape to the insert statement


			if (dump_format){
				printf("MULTILINESTRING(");
			}else{
				printf(",GeometryFromText('MULTILINESTRING (");
			}

			//for each vertice write out the coordinates in the insert statement, when there is a new line 
			//you must end the brackets and start new ones etc.
			for (u=0;u<obj->nVertices; u++){
				
				//check if the next vertice is the start of a new line				
				if(((next_ring != -99) && (u+1 == obj->panPartStart[next_ring] )) || u==(obj->nVertices-1) ){
					printf(",%.15g %.15g ",obj->padfX[u],obj->padfY[u]);
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
				printf(")',%s));\n",ARGV[3]);
			}

			
			SHPDestroyObject(obj);
		}

		if (!(dump_format) )
			printf("end;");//close last transaction
	
	
	}else if( obj->nSHPType == 11){  
		//---------------------------------------------------------------------------
		//------POINTZ (3D POINTS)---------------------------------------------------
		
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
				printf("%i",j);
			}else{
				printf("insert into %s values ('%i'",ARGV[2],j);
			}

			obj = SHPReadObject(hSHPHandle,j);

			Insert_attributes(hDBFHandle, ARGV,j);//add the attributes of each shape to the insert statement


			if (dump_format){
				printf("\tSRID=%s;MULTIPOINT(",ARGV[3]);
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
				printf(")',%s));\n",ARGV[3]);

			
			SHPDestroyObject(obj);
		}
		

	
		if (!(dump_format) )
			printf("end;");//end the last transaction
	}else{  
		printf ("");
		printf ("\n\n**** Type is NOT SUPPORTED, type id = %d ****\n\n",obj->nSHPType);
		//print out what type the file is and that it is not supported
	
	}//end the if statement for shape types

	}

if ((dump_format) )
	printf("\\.\n");
}//end main()


