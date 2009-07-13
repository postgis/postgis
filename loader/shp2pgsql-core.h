
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "shapefil.h"
#include "getopt.h"

#ifdef HAVE_ICONV
#include <iconv.h>
#endif

#include "../liblwgeom/liblwgeom.h"
#include "stringbuffer.h"

#define RCSID "$Id$"

#define TRANSLATION_IDLE 0
#define TRANSLATION_CREATE 1
#define TRANSLATION_LOAD 2
#define TRANSLATION_CLEANUP 3
#define TRANSLATION_DONE 4

enum {
	insert_null,
	skip_null,
	abort_on_null
};

/*
** Global variables for Core
*/
extern char opt; /* load mode: c = create, d = delete, a = append, p = prepare */
extern char *table; /* table to load into */
extern char *schema; /* schema to load into */
extern char *geom; /* geometry column name to use */
extern char *shp_file; /* the shape file (without the .shp extension) */
extern int dump_format; /* 0 = SQL inserts, 1 = dump */
extern int simple_geometries; /* 0 = MULTIPOLYGON/MULTILINESTRING, 1 = force to POLYGON/LINESTRING */
extern int quoteidentifiers; /* 0 = columnname, 1 = "columnName" */
extern int forceint4; /* 0 = allow int8 fields, 1 = no int8 fields */
extern int createindex; /* 0 = no index, 1 = create index after load */
extern int readshape; /* 0 = load DBF file only, 1 = load everything */
#ifdef HAVE_ICONV
extern char *encoding; /* iconv encoding name */
#endif
extern int null_policy; /* how to handle nulls */
extern int sr_id; /* SRID specified */
extern int gui_mode; /* 1 = GUI, 0 = commandline */
extern int translation_stage; /* 1 = ready, 2 = done start, 3 = done middle, 4 = done end */
extern int cur_entity; /* what record are we working on? */


/*
** Global variables used only by GUI
*/

/*
** Prototypes across modules
*/
extern int translation_start(void); 
extern int translation_middle(void); 
extern int translation_end(void); 
extern void pgui_log_va(const char *fmt, va_list ap);
extern int pgui_exec(const char *sql);
extern int pgui_copy_write(const char *line);
extern int pgui_copy_start(const char *sql);
extern int pgui_copy_end(const int rollback);
extern void LowerCase(char *s);

