//compile line for Refractions' Solaris machine...
//gcc -g -I/data3/postgresql-7.1.2/include -L/data3/postgresql-7.1.2/lib dump.c ../shapelib-1.2.8/shpopen.o ../shapelib-1.2.8/dbfopen.o -o dump -lpq

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "libpq-fe.h"
#include "shapefil.h"
#include "getopt.h"

static void exit_nicely(PGconn *conn){
	PQfinish(conn);
	exit(1);
}

int create_lines(char *str,int shape_id, SHPHandle shp,int dims);
int create_multilines(char *str,int shape_id, SHPHandle shp,int dims);
int create_points(char *str,SHPHandle shp,int dims);
int create_multipoints(char *str, SHPHandle shp,int dims);
int create_polygons(char *str,int shape_id, SHPHandle shp,int dims);
int create_multipolygons(char *str,int shape_id, SHPHandle shp,int dims);
int parse_points(char *str, int num_points, double *x,double *y,double *z);
int num_points(char *str);
int num_lines(char *str);
char *scan_to_same_level(char *str);
int points_per_sublist( char *str, int *npoints, long max_lists);



//main
//USAGE: pgsql2shp [<options>] <database> <table>
//OPTIONS:
//  -d Set the dump file to 3 dimensions, if this option is not used
//     all dumping will be 2d only.
//  -f <filename>  Use this option to specify the name of the file
//     to create.
//  -h <host>  Allows you to specify connection to a database on a
//     machine other than the localhost.
//  -p <port>  Allows you to specify a database port other than 5432.
//  -P <password>  Connect to the database with the specified password.
//  -u <user>  Connect to the database as the specified user.
//  -g <geometry_column> Specify the geometry column to be exported.

int main(int ARGC, char **ARGV){
	char	   *pghost,*pgport,*pgoptions,*dbName,*pgpass,
		   *query,*query1,*geo_str,*geo_str_left,
		   *geo_col_name,*geo_OID,
		   conn_string[512],field_name[32],table_OID[16],
		   *shp_file,*pguser, *table;
	int			nFields, is3d, c, errflg, curindex;
	int			i,j,type,size,flds;
	int			type_ary[256];
	int			OID,geovalue_field;

	DBFHandle	dbf;
	SHPHandle	shp;

	PGconn	   *conn;
	PGresult   *res,*res2,*res3;

	table = NULL;
	geo_col_name = NULL;
	geo_str = NULL;
	dbName = NULL;
	pghost = NULL;
	shp_file = NULL;
	pgport = NULL;
	geovalue_field = -1;
	pguser = "";
	pgpass = "";
	is3d = 0;
	errflg = 0;
	OID = 0;
        while ((c = getopt(ARGC, ARGV, "f:h:du:p:P:g:")) != EOF){
               switch (c) {
               case 'f':
                    shp_file = optarg;
                    break;
               case 'h':
                    pghost=optarg;
                    break;
               case 'd':
	            is3d = 1;
                    break;		  
               case 'u':
                    pguser = optarg;
                    break;
               case 'p':
                    pgport = optarg;
                    break;
	       case 'P':
		    pgpass = optarg;
		    break;
	       case 'g':
		    geo_col_name = optarg;
		    break;
               case '?':
                    errflg=1;
               }
        }

        curindex=0;
        for ( ; optind < ARGC; optind++){
                if(curindex ==0){
                        dbName = ARGV[optind];
                }else if(curindex == 1){
                        table = ARGV[optind];
                }
                curindex++;
        }
        if(curindex != 2){
                errflg = 1;
        }

        if (errflg==1) {
                printf("\n**ERROR** invalid option or command parameters\n");
                printf("\n");
                printf("USAGE: pgsql2shp [<options>] <database> <table>\n");
                printf("\n");
                printf("OPTIONS:\n");
                printf("  -d Set the dump file to 3 dimensions, if this option is not used\n");
                printf("     all dumping will be 2d only.\n");
                printf("  -f <filename>  Use this option to specify the name of the file\n");
                printf("     to create.\n");
                printf("  -h <host>  Allows you to specify connection to a database on a\n");
                printf("     machine other than the localhost.\n");
                printf("  -p <port>  Allows you to specify a database port other than 5432.\n");
                printf("  -P <password>  Connect to the database with the specified password.\n");
                printf("  -u <user>  Connect to the database as the specified user.\n");
		printf("  -g <geometry_column> Specify the geometry column to be exported.\n");
                printf("\n");
                exit (2);
        }

        if(shp_file == NULL){
		shp_file = malloc(strlen(table) + 1);
                strcpy(shp_file,table);
        }

        if(pgport == NULL){
		pgport = "5432";		
        }
        if(pghost == NULL){
		pghost = "localhost";		
        }

	if(strcmp(pgpass,"")==0 && strcmp(pguser,"")==0){
		pgoptions = malloc(1);
		strcpy(pgoptions,"");
	}else{
		pgoptions = malloc(strlen(pguser) + strlen(pgpass) + 20);
		if(strcmp(pguser,"")!=0){
			strcpy(pgoptions,"user=");
			strcat(pgoptions,pguser);
		}
		if(strcmp(pgpass,"") !=0 ){
			strcat(pgoptions," password=");
			strcat(pgoptions,pgpass); 
		}
	}

printf(conn_string);

	/* make a connection to the specified database */
	sprintf(conn_string,"host=%s %s port=%s dbname=%s",pghost,pgoptions,pgport,dbName);
	conn = PQconnectdb( conn_string );
	

	/* check to see that the backend connection was successfully made */
	if (PQstatus(conn) == CONNECTION_BAD)
	{
		fprintf(stderr, "Connection to database '%s' failed.\n", dbName);
		fprintf(stderr, "%s", PQerrorMessage(conn));
		exit_nicely(conn);
	}

#ifdef DEBUG
	debug = fopen("/tmp/trace.out", "w");
	PQtrace(conn, debug);
#endif	 /* DEBUG */


//------------------------------------------------------------
//Get the OID of the geometry type
	query1 = (char *)malloc(strlen("select OID, typname from pg_type where typname =  'geometry';")+2);
	strcpy(query1,"select OID, typname from pg_type where typname =  'geometry';");
	res = PQexec(conn, query1);	
	
	//	printf("OID query result: %s\n",PQresultErrorMessage(res));
	free (query1);
	if(PQntuples(res) > 0 ){
		char *temp_int = (char *)PQgetvalue(res, 0, 0);
		OID = atoi(temp_int);
	}else{
		printf("Cannot determine geometry type OID, Data-Dump Failed.");
		exit_nicely(conn);
	}
	PQclear(res);

//------------------------------------------------------------
//Get the table from the DB

	query= (char *)malloc(strlen(table) + strlen("select * from ")+2);
	strcpy(query, "select * from ") ;
	strcat(query, table);
	//	printf("%s\n",query);
	res = PQexec(conn, query);	

	if(PQntuples(res) > 0 ){

	}else{
		printf("Invalid table: '%s' (check spelling and existance of >0 tuples).\nData-Dump Failed.",table);
		exit_nicely(conn);
	}
	//printf("Select * query result: %s\n",PQresultErrorMessage(res));
	
	free (query);
	
//------------------------------------------------------------
//Create the dbf file

	dbf = DBFCreate(shp_file);
	if(dbf == NULL){
		printf("DBF could not be created - Dump FAILED.");
		exit_nicely(conn);
	}

	// add the fields to the DBF
	nFields = PQnfields(res);

	flds =0;		//keep track of how many fields you have actually created
	
	for (i = 0; i < nFields; i++){
		if(strlen(PQfname(res, i)) <32){
			strcpy(field_name, PQfname(res, i));
		}else{
			printf("field name %s is too long, must be less than 32 characters.\n",PQfname(res, i));
			exit_nicely(conn);
		}

		for(j=0;j<i;j++){ //make sure the fields all have unique names, 10-digit limit on dbf names...
			if(strncmp(field_name, PQfname(res, j),10) == 0){
				printf("\nWarning: Field '%s' has first 10 characters which duplicate a previous field name's.\nRenaming it to be: '",field_name);
				strncpy(field_name,field_name,9);
				field_name[9] = 0;
				sprintf(field_name,"%s%d",field_name,i);
				printf("%s'\n\n",field_name);
			}
		}

		type = PQftype(res,i);
		size = PQfsize(res,i);
		if(type == 1082){ //date field, which we store as a string so we need more width in the column
			size = 10;
		}
		if(size==-1 && type != OID){ //-1 represents variable size in postgres, this should not occur, but use 32 bytes in case it does
			
			//( this is ugly: don't forget counting the length 
			// when changing the fixed query strings )
			query1 = (char *)malloc(
			  24+strlen(PQfname(res, i))+8+strlen(table)+1 ); 
			
			strncpy(query1,"select max(octet_length(",24+1);
			strcat(query1,PQfname(res, i));
			strncat(query1,")) from ",8);
			strcat(query1,table);
			res2 = PQexec(conn, query1);			

			free(query1);
			if(PQntuples(res2) > 0 ){
				char *temp_int = (char *)PQgetvalue(res2, 0, 0);
				size = atoi(temp_int);
			}else{
				size = 32;
			}

		}
		if(type == 20 || type == 21 || type == 22 || type == 23){
			if(DBFAddField(dbf, field_name,FTInteger,16,0) == -1)printf("error - Field could not be created.\n");
			type_ary[i]=1;
			flds++;
		}else if(type == 700 || type == 701){
			if(DBFAddField(dbf, field_name,FTDouble,32,10) == -1)printf("error - Field could not be created.\n");
			type_ary[i]=2;
			flds++;
		}else if(type == OID){
		        if( geovalue_field == -1){
			        geovalue_field = i;
			        flds++;
			}else if(geo_col_name != NULL && !strcasecmp(geo_col_name, field_name)){
			        geovalue_field = i;
			}
			type_ary[i]=9; //the geometry type field			
		}else{
			if(DBFAddField(dbf, field_name,FTString,size,0) == -1)printf("error - Field could not be created.\n");
			type_ary[i]=3;
			flds++;
		}
	}


	
	/* next, print out the instances */

	for (i = 0; i < PQntuples(res); i++)
	{
		if(i%50 ==1){
			printf("DBF tuple - %d added\n",i);
		}
		flds = 0;			
		for (j = 0; j < nFields; j++){
			if(type_ary[j] == 1){
				char *temp_int = (char *)PQgetvalue(res, i, j);
				int temp = atoi(temp_int);
				if(DBFWriteIntegerAttribute(dbf, i, flds,temp)== 0)printf("error(int) - Record could not be created\n");
				flds++;
			}else if(type_ary[j] == 2){
				char *temp_dbl = PQgetvalue(res, i, j);
				double temp = atof(temp_dbl);
				if(DBFWriteDoubleAttribute( dbf, i, flds,temp)== 0)printf("error(double) - Record could not be created\n");
				flds++;
			}else if(type_ary[j] == 9){
				//this is the geometry type field, would do all the .shp and .shx stuff here i imagine
			}else{
				char *temp = (char *)malloc(strlen(PQgetvalue(res, i, j))*8);
				temp = (char *)PQgetvalue(res, i, j);
				if(DBFWriteStringAttribute( dbf, i, flds,temp)== 0)printf("error(string) - Record could not be created\n");
				flds++;
			}
			
		}
	
	}
	printf("DBF tuple - %d added\n",i);

	if(flds==0){
		printf("WARNING: There were no fields in the database. The DBF was not created properly, please add a field to the database and try again.");
	}

	DBFClose(dbf);




//--------------------------------------------------------------------
//Now parse the geo_value																													(All your base belong to us. For great justice.) 
//field into the shx and shp files


	query= (char *)malloc(strlen("select OID from pg_class where relname = ' '")+strlen(table)+2);
	strcpy(query, "select OID from pg_class where relname = '") ;
	strcat(query, table);
	strcat(query, "'");
	res3 = PQexec(conn, query);
	if(PQntuples(res3) == 1 ){
		strncpy(table_OID, (PQgetvalue(res3, 0,0)), 15 );
	}else if(PQntuples(res3) == 0 ){
		printf("ERROR: Cannot determine relation OID.\n");
		exit_nicely(conn);
	}else{
		strncpy(table_OID, (PQgetvalue(res3, 0,0)), 15 );
		printf("Warning: Multiple relations detected, the program will only dump the first relation.\n");
	}	

	//get the geometry column
	if(geo_col_name == NULL){
	        query= (char *)malloc(strlen("select attname from pg_attribute where attrelid =  and atttypid = ")+38);
		strcpy(query, "select attname from pg_attribute where attrelid = ");
		strcat(query, table_OID);
		strcat(query, " and atttypid = ");
		geo_OID = (char *)malloc(34);
		sprintf(geo_OID, "%i", OID);
		strcat(query, geo_OID );
	}else{
	        query = (char *)malloc(strlen("select attname from pg_attribute where attrelid = and atttypid = ")+
				       strlen("and attname = ''")+strlen(geo_col_name)+38);
		strcpy(query, "select attname from pg_attribute where attrelid = ");
		strcat(query, table_OID);
		strcat(query, " and atttypid = ");
		geo_OID = (char *)malloc(34);
		sprintf(geo_OID, "%i", OID);
		strcat(query, geo_OID );
		strcat(query, " and attname = '");
		strcat(query, geo_col_name);
		strcat(query, "'");
	}
	res3 = PQexec(conn, query);	
	if(PQntuples(res3) == 1 ){
		geo_col_name = (char *)malloc(strlen(PQgetvalue(res3, 0,0)) +2 );
		geo_col_name = PQgetvalue(res3,0,0);
	}else if(PQntuples(res3) == 0 ){
	        if(geo_col_name == NULL){
		        printf("ERROR: Cannot determine name of geometry column.\n");
		}else{
		        printf("ERROR: Wrong geometry column name.\n");
		}
		exit_nicely(conn);
	}else{
		geo_col_name = (char *)malloc(strlen(PQgetvalue(res3, 0,0)) +2 );
		geo_col_name = PQgetvalue(res3,0,0);
		printf("Warning: Multiple geometry columns detected, the program will only dump the first geometry.\n");
	}	



	//get what kind of Geometry type is in the table
	query= (char *)malloc(strlen(table) + strlen("select distinct (geometrytype()) from where NOT geometrytype(geom) = NULL")+18);
	strcpy(query, "select distinct (geometrytype(");
	strcat(query, geo_col_name);
	strcat(query, ")) from ") ;
	strcat(query, table);
	strcat(query," where NOT geometrytype(geom) = NULL");
	res3 = PQexec(conn, query);	
	//printf("\n\n-->%s\n\n",query);



	if(PQntuples(res3) == 1 ){
		geo_str = (char *)malloc(strlen(PQgetvalue(res3, 0, 0))+1);
		memcpy(geo_str, (char *)PQgetvalue(res3, 0, 0),strlen(PQgetvalue(res3,0,0))+1);
	}else if(PQntuples(res3) > 1 ){
		printf("ERROR: Cannot have multiple geometry types in a shapefile.\nUse option -t(unimplemented currently,sorry...) to specify what type of geometry you want dumped\n\n");
		exit_nicely(conn);
	}else{
		printf("ERROR: Cannot determine geometry type of table. \n");
		exit_nicely(conn);
	}
	
	geo_str_left = (char *)malloc(10);
	strncpy(geo_str_left,geo_str,8);
	free(geo_str);
	
	if(strncmp(geo_str_left,"MULTILIN",8)==0 ){			
		//multilinestring ---------------------------------------------------
		if(is3d == 0){
			shp = SHPCreate(shp_file, SHPT_ARC );//2d line shp file
		}else{
			shp = SHPCreate(shp_file, SHPT_ARCZ );//3d line shp file
		}
		
		for(i=0;i<PQntuples(res);i++){
			geo_str = (char *)malloc(strlen(PQgetvalue(res, i, geovalue_field))+1);
			memcpy(geo_str, (char *)PQgetvalue(res, i, geovalue_field),strlen(PQgetvalue(res, i, geovalue_field))+1);
			if(create_multilines(geo_str,i,shp,is3d) ==0){
				printf("Error writing multiline to shape file");
			}
			if(i%50 ==1){
				printf("shape - %d added\n",i);
			}
			free(geo_str);

		}
		printf("shape - %d added\n",i);

	}else if (strncmp(geo_str_left,"LINESTRI",8)==0 ){ 
		//linestring---------------------------------------------------
		if(is3d==0){
			shp = SHPCreate(shp_file, SHPT_ARC );//2d lines shp file
		}else{
			shp = SHPCreate(shp_file, SHPT_ARCZ );//3d lines shp file
		}
		
		for(i=0;i<PQntuples(res);i++){
			geo_str = (char *)malloc(strlen(PQgetvalue(res, i, geovalue_field))+1);
			memcpy(geo_str, (char *)PQgetvalue(res, i, geovalue_field),strlen(PQgetvalue(res, i, geovalue_field))+1);
			if(create_lines(geo_str,i,shp,is3d) ==0){
				printf("Error writing line to shape file");
			}
			if(i%100 ==0){
				printf("shape - %d added\n",i);
			}
			free(geo_str);
		}
		printf("shape - %d added\n",i);

	}else if (strncmp(geo_str_left,"POLYGON",7)==0 ){ 
		//Polygon---------------------------------------------------
		if(is3d==0){
			shp = SHPCreate(shp_file, SHPT_POLYGON );//2d lines shp file
		}else{
			shp = SHPCreate(shp_file, SHPT_POLYGONZ );//3d lines shp file
		}
		
		for(i=0;i<PQntuples(res);i++){
			geo_str = (char *)malloc(strlen(PQgetvalue(res, i, geovalue_field))+1);
			memcpy(geo_str, (char *)PQgetvalue(res, i, geovalue_field),strlen(PQgetvalue(res, i, geovalue_field))+1);
			if(create_polygons(geo_str,i,shp,is3d) == 0){
				printf("Error writing polygon to shape file");
			}
			if(i%100 == 0){
				printf("shape - %d added\n",i);
			}
			free(geo_str);
		}
		printf("shape - %d added\n",i);
	}else if(strncmp(geo_str_left,"MULTIPOL",8)==0 ){	
		//multipolygon---------------------------------------------------
		if(is3d==0){
			shp = SHPCreate(shp_file, SHPT_POLYGON );//2d polygon shp file
		}else{
			shp = SHPCreate(shp_file, SHPT_POLYGONZ );//3d polygon shp file
		}
		for(i=0;i<PQntuples(res);i++){
			geo_str = (char *)malloc(strlen(PQgetvalue(res, i, geovalue_field))+1);
			memcpy(geo_str, (char *)PQgetvalue(res, i, geovalue_field),strlen(PQgetvalue(res, i, geovalue_field))+1);
			if(create_multipolygons(geo_str,i,shp,is3d) == 0){
				printf("Error writing polygon to shape file");
			}
			if(i%100 == 0){
				printf("shape - %d added\n",i);
			}
			free(geo_str);
		}
		printf("shape - %d added\n",i);
	}else if(strncmp(geo_str_left,"POINT",5)==0 ){	
		//point---------------------------------------------------
		if(is3d==0){
			shp = SHPCreate(shp_file, SHPT_POINT );//2d point shp file
		}else{
			shp = SHPCreate(shp_file, SHPT_POINTZ );//3d point shp file
		}
		
		for(i=0;i<PQntuples(res);i++){
			geo_str = (char *)malloc(strlen(PQgetvalue(res, i, geovalue_field))+1);
			memcpy(geo_str, (char *)PQgetvalue(res, i, geovalue_field),strlen(PQgetvalue(res, i, geovalue_field))+1);
			if(create_points(geo_str,shp,is3d) ==0){
				printf("Error writing line to shape file");
			}
			if(i%100 ==0){
				printf("shape - %d added\n",i);
			}
			free(geo_str);
		}
		printf("shape - %d added\n",i);
	}else if(strncmp(geo_str_left,"MULTIPOI",8)==0 ){	
		//multipoint---------------------------------------------------

		if(is3d==0){
			shp = SHPCreate(shp_file, SHPT_MULTIPOINT );//2d point shp file
		}else{
			shp = SHPCreate(shp_file, SHPT_MULTIPOINTZ );//3d point shp file
		}
		
		for(i=0;i<PQntuples(res);i++){
			geo_str = (char *)malloc(strlen(PQgetvalue(res, i, geovalue_field))+1);
			memcpy(geo_str, (char *)PQgetvalue(res, i, geovalue_field),strlen(PQgetvalue(res, i, geovalue_field))+1);
			if(create_multipoints(geo_str,shp,is3d) ==0){
				printf("Error writing line to shape file");
			}
			if(i%100 ==0){
				printf("shape - %d added\n",i);
			}
			free(geo_str);
		}
		printf("shape - %d added\n",i);
	}else{
		printf("type '%s' is not Supported at this time.\nThe DBF file has been created but not the shx or shp files.\n",geo_str_left);
		shp = NULL;
	}


	if(shp != NULL){	
		SHPClose( shp );	
	}
	


	free(geo_str_left);

	PQclear(res);
	/* close the connection to the database and cleanup */
	PQfinish(conn);

#ifdef DEBUG
	fclose(debug);
#endif	 /* DEBUG */

	return 0;
}





//reads points into x,y,z co-ord arrays
int parse_points(char *str, int num_points, double *x,double *y,double *z){
	int	keep_going;
	int	num_found= 0;
	char	*end_of_double;

	if ( (str == NULL) || (str[0] == 0) ){
		return 0;  //either null string or empty string
	}
	
	//look ahead for the "("
	str = strchr(str,'(') ;
	
	if ( (str == NULL) || (str[1] == 0) ){  // str[0] = '(';
		return 0;  //either didnt find "(" or its at the end of the string
	}
	str++;  //move forward one char
	keep_going = 1;
	while (keep_going == 1){
		
		//attempt to get the point
		//scanf is slow, so we use strtod()

		x[num_found] = (double)strtod(str,&end_of_double);
		if (end_of_double == str){
			return 0; //error occured (nothing parsed)
		}
		str = end_of_double;
		y[num_found] = strtod(str,&end_of_double);
		if (end_of_double == str){
			return 0; //error occured (nothing parsed)
		}
		str = end_of_double;
		z[num_found] = strtod(str,&end_of_double); //will be zero if error occured
		str = end_of_double;
		num_found++;

		str=strpbrk(str,",)");  // look for a "," or ")"
		if (str != NULL && str[0] == ','){
			str++;
		}
		keep_going = (str != NULL) && (str[0] != ')');		
	}
	return num_found; 
}





//returns how many points are in the first list in str
//
//  1. scan ahead looking for "("
//  2. find "," until hit a ")"
//  3. return number of points found
//	
// NOTE: doesnt actually parse the points, so if the 
//       str contains an invalid geometry, this could give
// 	   back the wrong answer.
//
// "(1 2 3, 4 5 6),(7 8, 9 10, 11 12 13)" => 2 (2nd list is not included)
int	num_points(char *str){
	int		keep_going;
	int		points_found = 1; //no "," if only one point (and last point)
						

	if ( (str == NULL) || (str[0] == 0) )
	{
		return 0;  //either null string or empty string
	}

	//look ahead for the "("

	str = strchr(str,'(') ;
	
	if ( (str == NULL) || (str[1] == 0) )  // str[0] = '(';
	{
		return 0;  //either didnt find "(" or its at the end of the string
	}

	keep_going = 1;
	while (keep_going) 
	{
		str=strpbrk(str,",)");  // look for a "," or ")"
		keep_going = (str != NULL);
		if (keep_going)  // found a , or )
		{
			if (str[0] == ')')
			{
				//finished
				return points_found;
			}
			else	//str[0] = ","
			{
				points_found++;
				str++; //move 1 char forward	
			}
		}
	}
	return points_found; // technically it should return an error.
}

//number of sublist in a string.

// Find the number of lines in a Multiline
// OR
// The number of rings in a Polygon
// OR
// The number of polygons in a multipolygon

// ( (..),(..),(..) )  -> 3
// ( ( (..),(..) ), ( (..) )) -> 2
// ( ) -> 0
// scan through the list, for every "(", depth (nesting) increases by 1
//				  for every ")", depth (nesting) decreases by 1
// if find a "(" at depth 1, then there is a sub list
//
// example: 
//      "(((..),(..)),((..)))"
//depth  12333223332112333210
//        +           +         increase here

int num_lines(char *str){
	int	current_depth = 0;
	int	numb_lists = 0;


	while ( (str != NULL) && (str[0] != 0) )
	{
		str=strpbrk(str,"()"); //look for "(" or ")"
		if (str != NULL)
		{
			if (str[0] == '(')
			{
				current_depth++;
				if (current_depth == 2)
					numb_lists ++;
			}
			if (str[0] == ')')
			{
				current_depth--;
				if (current_depth == 0)
					return numb_lists ;
			}
			str++;
		}
	}
	return numb_lists ; // probably should give an error
}



//simple scan-forward to find the next "(" at the same level
//  ( (), (),(), ),(...
//                 + return this location
char *scan_to_same_level(char *str){

	//scan forward in string looking for at "(" at the same level
	// as the one its already pointing at

	int	current_depth = 0;
	int  first_one=1;


	while ( (str != NULL) && (str[0] != 0) )
	{
		str=strpbrk(str,"()");
		if (str != NULL)
		{
			if (str[0] == '(')
			{
				if (!(first_one))
				{
					if (current_depth == 0)
						return str;
				}
				else
					first_one = 0;  //ignore the first opening "("
				current_depth++;
			}
			if (str[0] == ')')
			{
				current_depth--;
			}

			str++;
		}
	}
	return str ; // probably should give an error
}






// Find out how many points are in each sublist, put the result in the array npoints[]
//  (for at most max_list sublists)
//
//  ( (L1),(L2),(L3) )  --> npoints[0] = points in L1,
//				    npoints[1] = points in L2,
//				    npoints[2] = points in L3
//
// We find these by, again, scanning through str looking for "(" and ")"
// to determine the current depth.  We dont actually parse the points.

int points_per_sublist( char *str, int *npoints, long max_lists){
	//scan through, noting depth and ","s

	int	current_depth = 0;
	int	current_list =-1 ;


	while ( (str != NULL) && (str[0] != 0) )
	{
		str=strpbrk(str,"(),");  //find "(" or ")" or ","
		if (str != NULL)
		{
			if (str[0] == '(')
			{
				current_depth++;
				if (current_depth == 2)
				{
					current_list ++;
					if (current_list >=max_lists)
						return 1;			// too many sub lists found
					npoints[current_list] = 1;
				}
				// might want to return an error if depth>2
			}
			if (str[0] == ')')
			{
				current_depth--;
				if (current_depth == 0)
					return 1 ;
			}
			if (str[0] == ',')
			{
				if (current_depth==2)
				{
					npoints[current_list] ++;	
				}
			}

			str++;
		}
	}
	return 1 ; // probably should give an error
}



int create_multilines(char *str,int shape_id, SHPHandle shp,int dims){
	int		lines,i,j,max_points,index;
	int		*points;
	int		*part_index;
	
	double	*x;
	double	*y;
	double	*z;

	double	*totx;
	double	*toty;
	double	*totz;
	SHPObject *obj;

	
	lines = num_lines(str);
	
	points = (int *)malloc(sizeof(int) * lines);

	if(points_per_sublist(str, points, lines) ==0){
		printf("error - points_per_sublist failed");
	}
	max_points = 0;
	for(j=0;j<lines;j++){
		max_points += points[j];
	}
		
	part_index = (int *)malloc(sizeof(int) * lines);
	totx = (double *)malloc(sizeof(double) * max_points);
	toty = (double *)malloc(sizeof(double) * max_points);
	totz = (double *)malloc(sizeof(double) * max_points);
	
	index=0;

	for(i=0;i<lines;i++){
		str = strchr(str,'(') ;

		
		if(str[0] =='(' && str[1] == '('){
			str++;
		}
		
		x = (double *)malloc(sizeof(double) * points[i]);
		y = (double *)malloc(sizeof(double) * points[i]);
		z = (double *)malloc(sizeof(double) * points[i]);

		parse_points(str,points[i],x,y,z);
		str = scan_to_same_level(str);
		part_index[i] = index;
		for(j=0;j<points[i];j++){
			totx[index] = x[j];
			toty[index] = y[j];
			totz[index] = z[j];
			index++;
		}
		free(x);
		free(y);
		free(z);
	}

	
	obj = (SHPObject *)malloc(sizeof(SHPObject));
	if(dims == 0){
		obj = SHPCreateObject(SHPT_ARC,shape_id,lines,part_index,NULL,max_points,totx,toty,totz,NULL);
	}else{
		obj = SHPCreateObject(SHPT_ARCZ,shape_id,lines,part_index,NULL,max_points,totx,toty,totz,NULL);
	}
	free(part_index);
	free(points);
	free(totx);
	free(toty);
	free(totz);
	if(SHPWriteObject(shp,-1,obj)!= -1){
		SHPDestroyObject(obj);
		return 1;
	}else{
		SHPDestroyObject(obj);
		return 0;
	}
}


int create_lines(char *str,int shape_id, SHPHandle shp,int dims){
	int		points;
	int		*part_index;
	
	double	*x,
			*y,
			*z;
	SHPObject	*obj;

	
	part_index = (int *)malloc(sizeof(int));	//we know lines only have 1 part so make the array of size 1
	part_index[0] = 0;

	points = num_points(str);
	x = (double *)malloc(sizeof(double) * points);
	y = (double *)malloc(sizeof(double) * points);
	z = (double *)malloc(sizeof(double) * points);

	parse_points(str,points,x,y,z);

	obj = (SHPObject *)malloc(sizeof(SHPObject));
	
	if(dims == 0){
		obj = SHPCreateObject(SHPT_ARC,shape_id,1,part_index,NULL,points,x,y,z,NULL);
	}else{
		obj = SHPCreateObject(SHPT_ARCZ,shape_id,1,part_index,NULL,points,x,y,z,NULL);
	}
	free(part_index);
	free(x);
	free(y);
	free(z);
	if(SHPWriteObject(shp,-1,obj)!= -1){
		SHPDestroyObject(obj);
		return 1;
	}else{
		SHPDestroyObject(obj);
		return 0;
	}
}



int create_points(char *str, SHPHandle shp,int dims){
	
	double	*x,
			*y,
			*z;
	SHPObject	*obj;
	int notnull;
	notnull = 1;
	
	x = (double *)malloc(sizeof(double));
	y = (double *)malloc(sizeof(double));
	z = (double *)malloc(sizeof(double));

	notnull = parse_points(str,1,x,y,z);
	

	obj = (SHPObject *)malloc(sizeof(SHPObject));
	
	if(dims == 0){
		if(notnull == 1){
			obj = SHPCreateSimpleObject(SHPT_POINT,1,x,y,z);
		}else{
			obj = SHPCreateSimpleObject(SHPT_NULL,1,x,y,z);
		}
	}else{
		if(notnull == 1){
			obj = SHPCreateSimpleObject(SHPT_POINTZ,1,x,y,z);
		}else{
			obj = SHPCreateSimpleObject(SHPT_NULL,1,x,y,z);
		}
	}
	free(x);
	free(y);
	free(z);
	if(SHPWriteObject(shp,-1,obj)!= -1){
		SHPDestroyObject(obj);
		return 1;
	}else{
		SHPDestroyObject(obj);
		return 0;
	}
}



int create_multipoints(char *str, SHPHandle shp,int dims){
	int points;	
	
	double	*x,
			*y,
			*z;
	SHPObject	*obj;
	int notnull;
	notnull=1;	

	points = num_points(str);
	x = (double *)malloc(sizeof(double)*points);
	y = (double *)malloc(sizeof(double)*points);
	z = (double *)malloc(sizeof(double)*points);

	notnull = parse_points(str,points,x,y,z);

	obj = (SHPObject *)malloc(sizeof(SHPObject));
	
	if(dims == 0){
		if(notnull == 1){
			obj = SHPCreateSimpleObject(SHPT_MULTIPOINT ,1,x,y,z);
		}else{
			obj = SHPCreateSimpleObject(SHPT_NULL ,1,x,y,z);
		}
	}else{
		if(notnull == 1){
			obj = SHPCreateSimpleObject(SHPT_MULTIPOINTZ ,1,x,y,z);
		}else{
			obj = SHPCreateSimpleObject(SHPT_NULL ,1,x,y,z);
		}
	}

	free(x);
	free(y);
	free(z);
	if(SHPWriteObject(shp,-1,obj)!= -1){
		SHPDestroyObject(obj);
		return 1;
	}else{
		SHPDestroyObject(obj);
		return 0;
	}
}




int create_polygons(char *str,int shape_id, SHPHandle shp,int dims){
	int		rings,i,j,max_points,index;
	int		*points;
	int		*part_index;
	
	double	*x;
	double	*y;
	double	*z;

	double	*totx;
	double	*toty;
	double	*totz;
	SHPObject *obj;

	rings = num_lines(str); //the number of rings in the polygon
	points = (int *)malloc(sizeof(int) * rings);

	if(points_per_sublist(str, points, rings) ==0){
		printf("error - points_per_sublist failed");
	}
	max_points = 0;
	for(j=0;j<rings;j++){
		max_points += points[j];
	}
		
	part_index = (int *)malloc(sizeof(int) * rings);
	totx = (double *)malloc(sizeof(double) * max_points);
	toty = (double *)malloc(sizeof(double) * max_points);
	totz = (double *)malloc(sizeof(double) * max_points);
	
	index=0;

	for(i=0;i<rings;i++){
		str = strchr(str,'(') ;

		
		if(str[0] =='(' && str[1] == '('){
			str++;
		}
		
		x = (double *)malloc(sizeof(double) * points[i]);
		y = (double *)malloc(sizeof(double) * points[i]);
		z = (double *)malloc(sizeof(double) * points[i]);

		parse_points(str,points[i],x,y,z);
		str = scan_to_same_level(str);
		part_index[i] = index;
		for(j=0;j<points[i];j++){
			totx[index] = x[j];
			toty[index] = y[j];
			totz[index] = z[j];
			index++;
		}
		free(x);
		free(y);
		free(z);
	}

	obj = (SHPObject *)malloc(sizeof(SHPObject));
	if(dims == 0){
		obj = SHPCreateObject(SHPT_POLYGON,shape_id,rings,part_index,NULL,max_points,totx,toty,totz,NULL);
	}else{
		obj = SHPCreateObject(SHPT_POLYGONZ,shape_id,rings,part_index,NULL,max_points,totx,toty,totz,NULL);
	}
	free(part_index);
	free(points);
	free(totx);
	free(toty);
	free(totz);
	if(SHPWriteObject(shp,-1,obj)!= -1){
		SHPDestroyObject(obj);
		return 1;
	}else{
		SHPDestroyObject(obj);
		return 0;
	}
}


int create_multipolygons(char *str,int shape_id, SHPHandle shp,int dims){
	int		polys,rings,i,j,k,max_points;
	int		index,indexk,index2part,tot_rings,final_max_points;
	int		*points;
	int		*part_index;
	int		*final_part_index;
	
	char	*temp;
	char	*temp_addr;

	double	*x;
	double	*y;
	double	*z;

	double	*totx;
	double	*toty;
	double	*totz;

	double	*finalx;
	double	*finaly;
	double	*finalz;

	SHPObject *obj;
	
	points=0;

	polys = num_lines(str); //the number of rings in the polygon
	final_max_points =0;

	temp_addr = (char *)malloc(strlen(str) +1 );
	temp = temp_addr; //keep original pointer to free the mem later
	strcpy(temp,str);
	tot_rings=0;
	index2part=0;
	indexk=0;
	

	for(i=0;i<polys;i++){
		temp = strstr(temp,"((") ;
		if(temp[0] =='(' && temp[1] == '(' && temp[2] =='('){
			temp++;
		}
		rings = num_lines(temp);
		points = (int *)malloc(sizeof(int) * rings);
		points_per_sublist(temp, points, rings);
		tot_rings += rings;
		for(j=0;j<rings;j++){
			final_max_points += points[j];
		}
		
		temp+= 2;
	}
	free(points);
	temp= temp_addr;
	
	final_part_index = (int *)malloc(sizeof(int) * tot_rings);	
	finalx = (double *)malloc(sizeof(double) * final_max_points); 
	finaly = (double *)malloc(sizeof(double) * final_max_points); 
	finalz = (double *)malloc(sizeof(double) * final_max_points); 


	for(k=0;k<polys ; k++){ //for each polygon  

		str = strstr(str,"((") ;
		if(strlen(str) >2 && str[0] =='(' && str[1] == '(' && str[2] =='('){
			str++;
		}

		rings = num_lines(str);
		points = (int *)malloc(sizeof(int) * rings);

		if(points_per_sublist(str, points, rings) ==0){
			printf("error - points_per_sublist failed");
		}
		max_points = 0;
		for(j=0;j<rings;j++){
			max_points += points[j];
		}
		
		part_index = (int *)malloc(sizeof(int) * rings);
		totx = (double *)malloc(sizeof(double) * max_points);
		toty = (double *)malloc(sizeof(double) * max_points);
		totz = (double *)malloc(sizeof(double) * max_points);
	
		index=0;

		for(i=0;i<rings;i++){
			str = strchr(str,'(') ;
			if(str[0] =='(' && str[1] == '('){
				str++;
			}
			
			x = (double *)malloc(sizeof(double) * points[i]);
			y = (double *)malloc(sizeof(double) * points[i]);
			z = (double *)malloc(sizeof(double) * points[i]);

			parse_points(str,points[i],x,y,z);
			str = scan_to_same_level(str);
			part_index[i] = index;
			for(j=0;j<points[i];j++){
				totx[index] = x[j];
				toty[index] = y[j];
				totz[index] = z[j];
				index++;
			}
			free(x);
			free(y);
			free(z);
		}
		for(j=0;j<i;j++){
			final_part_index[index2part] = part_index[j]+indexk ;
			index2part++;
		}

		for(j=0;j<index;j++){
			finalx[indexk] = totx[j];
			finaly[indexk] = toty[j];
			finalz[indexk] = totz[j];
			indexk++;
		}
		
		free(points);
		free(part_index);
		free(totx);
		free(toty);
		free(totz);
		str -= 1;
	}//end for (k...


	obj	= (SHPObject *)malloc(sizeof(SHPObject));
	if(dims == 0){
		obj = SHPCreateObject(SHPT_POLYGON,shape_id,tot_rings,final_part_index,NULL,final_max_points,finalx,finaly,finalz,NULL);
	}else{
		obj = SHPCreateObject(SHPT_POLYGONZ,shape_id,tot_rings,final_part_index,NULL,final_max_points,finalx,finaly,finalz,NULL);
	}
	
	free(final_part_index);
	free(finalx);
	free(finaly);
	free(finalz);
	if(SHPWriteObject(shp,-1,obj)!= -1){
		SHPDestroyObject(obj);
		return 1;
	}else{
		SHPDestroyObject(obj);
		return 0;
	}
	
}
