/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * Copyright (C) 2015 Sandro Santilli <strk@kbt.io>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "postgres.h"
#include "fmgr.h"
#include "c.h" /* for UINT64_FORMAT and uint64 */
#include "utils/builtins.h"
#include "utils/elog.h"
#include "utils/memutils.h" /* for TopMemoryContext */
#include "utils/array.h" /* for ArrayType */
#include "catalog/pg_type.h" /* for INT4OID */
#include "lib/stringinfo.h"
#include "access/htup_details.h" /* for heap_form_tuple() */
#include "access/xact.h" /* for RegisterXactCallback */
#include "funcapi.h" /* for FuncCallContext */
#include "executor/spi.h" /* this is what you need to work with SPI */
#include "inttypes.h" /* for PRId64 */
#include "../postgis_config.h"

#include "liblwgeom_internal.h" /* for gbox_clone */
#include "liblwgeom_topo.h"

/*#define POSTGIS_DEBUG_LEVEL 1*/
#include "lwgeom_log.h"
#include "lwgeom_pg.h"
#include "pgsql_compat.h"

#include <stdarg.h>

#ifndef __GNUC__
# define __attribute__ (x)
#endif

#define ABS(x) (x<0?-x:x)

#ifdef WIN32
# define LWTFMT_ELEMID "lld"
#else
# define LWTFMT_ELEMID PRId64
#endif

/*
 * This is required for builds against pgsql
 */
PG_MODULE_MAGIC;

LWT_BE_IFACE* be_iface;

/*
 * Private data we'll use for this backend
 */
#define MAXERRLEN 256
struct LWT_BE_DATA_T
{
  char lastErrorMsg[MAXERRLEN];
  /*
   * This flag will need to be set to false
   * at top-level function enter and set true
   * whenever an callback changes the data
   * in the database.
   * It will be used by SPI_execute calls to
   * make sure to see any data change occurring
   * doring operations.
   */
  bool data_changed;

  int topoLoadFailMessageFlavor; /* 0:sql, 1:AddPoint */
};

LWT_BE_DATA be_data;

struct LWT_BE_TOPOLOGY_T
{
  LWT_BE_DATA* be_data;
  char *name;
  int id;
  int32_t srid;
  double precision;
  int hasZ;
  Oid geometryOID;
};

/* utility funx */

static void cberror(const LWT_BE_DATA* be, const char *fmt, ...)
__attribute__ (( format(printf, 2, 3) ));

static void
cberror(const LWT_BE_DATA* be_in, const char *fmt, ...)
{
  LWT_BE_DATA *be = (LWT_BE_DATA*)be_in;/*const cast*/
  va_list ap;

  va_start(ap, fmt);

  vsnprintf (be->lastErrorMsg, MAXERRLEN, fmt, ap);
  be->lastErrorMsg[MAXERRLEN-1]='\0';

  va_end(ap);
}

static void
_lwtype_upper_name(int type, char *buf, size_t buflen)
{
  char *ptr;
  snprintf(buf, buflen, "%s", lwtype_name(type));
  buf[buflen-1] = '\0';
  ptr = buf;
  while (*ptr)
  {
    *ptr = toupper(*ptr);
    ++ptr;
  }
}

/* Return an lwalloc'ed geometrical representation of the box */
static LWGEOM *
_box2d_to_lwgeom(const GBOX *bbox, int32_t srid)
{
  POINTARRAY *pa = ptarray_construct(0, 0, 2);
  POINT4D p;
  LWLINE *line;

  p.x = bbox->xmin;
  p.y = bbox->ymin;
  ptarray_set_point4d(pa, 0, &p);
  p.x = bbox->xmax;
  p.y = bbox->ymax;
  ptarray_set_point4d(pa, 1, &p);
  line = lwline_construct(srid, NULL, pa);
  return lwline_as_lwgeom(line);
}

/* Return lwalloc'ed hexwkb representation for a GBOX */
static char *
_box2d_to_hexwkb(const GBOX *bbox, int32_t srid)
{
	char *hex;
	LWGEOM *geom = _box2d_to_lwgeom(bbox, srid);
	hex = lwgeom_to_hexwkb_buffer(geom, WKT_EXTENDED);
	lwgeom_free(geom);
	return hex;
}

/* Backend callbacks */

static const char*
cb_lastErrorMessage(const LWT_BE_DATA* be)
{
  return be->lastErrorMsg;
}

static LWT_BE_TOPOLOGY*
cb_loadTopologyByName(const LWT_BE_DATA* be, const char *name)
{
  int spi_result;
  const char *sql;
  Datum dat;
  bool isnull;
  LWT_BE_TOPOLOGY *topo;
  MemoryContext oldcontext = CurrentMemoryContext;
  Datum values[1];
  Oid argtypes[1];
  static SPIPlanPtr plan = NULL;

  argtypes[0] = CSTRINGOID;
  sql =
    "SELECT id,srid,precision,null::geometry "
    "FROM topology.topology WHERE name = $1::varchar";
  if ( ! plan ) /* prepare on first call */
  {
    plan = SPI_prepare(sql, 1, argtypes);
    if ( ! plan )
    {
      cberror(be, "unexpected return (%d) from query preparation: %s",
              SPI_result, sql);
      return NULL;
    }
    SPI_keepplan(plan);
    /* SPI_freeplan to free, eventually */
  }

  /* execute */
  values[0] = CStringGetDatum(name);
  spi_result = SPI_execute_plan(plan, values, NULL, !be->data_changed, 1);
  MemoryContextSwitchTo( oldcontext ); /* switch back */
  if ( spi_result != SPI_OK_SELECT )
  {
    cberror(be, "unexpected return (%d) from query execution: %s", spi_result, sql);
    return NULL;
  }
  if ( ! SPI_processed )
  {
    if ( be->topoLoadFailMessageFlavor == 1 )
    {
      cberror(be, "No topology with name \"%s\" in topology.topology", name);
    }
    else
    {
      cberror(be, "SQL/MM Spatial exception - invalid topology name");
    }
    return NULL;
  }
  if ( SPI_processed > 1 )
  {
    cberror(be, "multiple topologies named '%s' were found", name);
    return NULL;
  }

  topo = palloc(sizeof(LWT_BE_TOPOLOGY));
  topo->be_data = (LWT_BE_DATA *)be; /* const cast.. */
  topo->name = pstrdup(name);
  topo->hasZ = 0;

  dat = SPI_getbinval(SPI_tuptable->vals[0], SPI_tuptable->tupdesc, 1, &isnull);
  if ( isnull )
  {
    cberror(be, "Topology '%s' has null identifier", name);
    SPI_freetuptable(SPI_tuptable);
    return NULL;
  }
  topo->id = DatumGetInt32(dat);

  dat = SPI_getbinval(SPI_tuptable->vals[0], SPI_tuptable->tupdesc, 2, &isnull);
  if ( isnull )
  {
    cberror(be, "Topology '%s' has null SRID", name);
    SPI_freetuptable(SPI_tuptable);
    return NULL;
  }
  topo->srid = DatumGetInt32(dat);
  if ( topo->srid < 0 )
  {
    lwnotice("Topology SRID value %d converted to "
             "the officially unknown SRID value %d", topo->srid, SRID_UNKNOWN);
    topo->srid = SRID_UNKNOWN;
  }

  dat = SPI_getbinval(SPI_tuptable->vals[0], SPI_tuptable->tupdesc, 3, &isnull);
  if ( isnull )
  {
    lwnotice("Topology '%s' has null precision, taking as 0", name);
    topo->precision = 0; /* TODO: should this be -1 instead ? */
  }
  else
  {
    topo->precision = DatumGetFloat8(dat);
  }

  /* we're dynamically querying geometry type here */
  topo->geometryOID = TupleDescAttr(SPI_tuptable->tupdesc, 3)->atttypid;

  POSTGIS_DEBUGF(1, "cb_loadTopologyByName: topo '%s' has "
                 "id %d, srid %d, precision %g",
                 name, topo->id, topo->srid, topo->precision);

  SPI_freetuptable(SPI_tuptable);

  return topo;
}

static int
cb_topoGetSRID(const LWT_BE_TOPOLOGY* topo)
{
  return topo->srid;
}

static int
cb_topoHasZ(const LWT_BE_TOPOLOGY* topo)
{
  return topo->hasZ;
}

static double
cb_topoGetPrecision(const LWT_BE_TOPOLOGY* topo)
{
  return topo->precision;
}

static int
cb_freeTopology(LWT_BE_TOPOLOGY* topo)
{
  pfree(topo->name);
  pfree(topo);
  return 1;
}

static void
addEdgeFields(StringInfo str, int fields, int fullEdgeData)
{
  const char *sep = "";

  if ( fields & LWT_COL_EDGE_EDGE_ID )
  {
    appendStringInfoString(str, "edge_id");
    sep = ",";
  }
  if ( fields & LWT_COL_EDGE_START_NODE )
  {
    appendStringInfo(str, "%sstart_node", sep);
    sep = ",";
  }
  if ( fields & LWT_COL_EDGE_END_NODE )
  {
    appendStringInfo(str, "%send_node", sep);
    sep = ",";
  }
  if ( fields & LWT_COL_EDGE_FACE_LEFT )
  {
    appendStringInfo(str, "%sleft_face", sep);
    sep = ",";
  }
  if ( fields & LWT_COL_EDGE_FACE_RIGHT )
  {
    appendStringInfo(str, "%sright_face", sep);
    sep = ",";
  }
  if ( fields & LWT_COL_EDGE_NEXT_LEFT )
  {
    appendStringInfo(str, "%snext_left_edge", sep);
    if ( fullEdgeData ) appendStringInfoString(str, ", abs_next_left_edge");
    sep = ",";
  }
  if ( fields & LWT_COL_EDGE_NEXT_RIGHT )
  {
    appendStringInfo(str, "%snext_right_edge", sep);
    if ( fullEdgeData ) appendStringInfoString(str, ", abs_next_right_edge");
    sep = ",";
  }
  if ( fields & LWT_COL_EDGE_GEOM )
  {
    appendStringInfo(str, "%sgeom", sep);
  }
}

/* Add edge values in text form, include the parens */
static void
addEdgeValues(StringInfo str, const LWT_ISO_EDGE *edge, int fields, int fullEdgeData)
{
  char *hexewkb;
  const char *sep = "";

  appendStringInfoChar(str, '(');
  if ( fields & LWT_COL_EDGE_EDGE_ID )
  {
    if ( edge->edge_id != -1 )
      appendStringInfo(str, "%" LWTFMT_ELEMID, edge->edge_id);
    else
      appendStringInfoString(str, "DEFAULT");
    sep = ",";
  }
  if ( fields & LWT_COL_EDGE_START_NODE )
  {
    appendStringInfo(str, "%s%" LWTFMT_ELEMID, sep, edge->start_node);
    sep = ",";
  }
  if ( fields & LWT_COL_EDGE_END_NODE )
  {
    appendStringInfo(str, "%s%" LWTFMT_ELEMID, sep, edge->end_node);
    sep = ",";
  }
  if ( fields & LWT_COL_EDGE_FACE_LEFT )
  {
    appendStringInfo(str, "%s%" LWTFMT_ELEMID, sep, edge->face_left);
    sep = ",";
  }
  if ( fields & LWT_COL_EDGE_FACE_RIGHT )
  {
    appendStringInfo(str, "%s%" LWTFMT_ELEMID, sep, edge->face_right);
    sep = ",";
  }
  if ( fields & LWT_COL_EDGE_NEXT_LEFT )
  {
    appendStringInfo(str, "%s%" LWTFMT_ELEMID, sep, edge->next_left);
    if ( fullEdgeData )
      appendStringInfo(str, ",%" LWTFMT_ELEMID, ABS(edge->next_left));
    sep = ",";
  }
  if ( fields & LWT_COL_EDGE_NEXT_RIGHT )
  {
    appendStringInfo(str, "%s%" LWTFMT_ELEMID, sep, edge->next_right);
    if ( fullEdgeData )
      appendStringInfo(str, ",%" LWTFMT_ELEMID, ABS(edge->next_right));
    sep = ",";
  }
  if ( fields & LWT_COL_EDGE_GEOM )
  {
    if ( edge->geom )
    {
	    hexewkb = lwgeom_to_hexwkb_buffer(lwline_as_lwgeom(edge->geom), WKB_EXTENDED);
	    appendStringInfo(str, "%s'%s'::geometry", sep, hexewkb);
	    lwfree(hexewkb);
    }
    else
    {
      appendStringInfo(str, "%snull", sep);
    }
  }
  appendStringInfoChar(str, ')');
}

enum UpdateType
{
  updSet,
  updSel,
  updNot
};

static void
addEdgeUpdate(StringInfo str, const LWT_ISO_EDGE* edge, int fields,
              int fullEdgeData, enum UpdateType updType)
{
  const char *sep = "";
  const char *sep1;
  const char *op;
  char *hexewkb;

  switch (updType)
  {
  case updSet:
    op = "=";
    sep1 = ",";
    break;
  case updSel:
    op = "=";
    sep1 = " AND ";
    break;
  case updNot:
  default:
    op = "!=";
    sep1 = " AND ";
    break;
  }

  if ( fields & LWT_COL_EDGE_EDGE_ID )
  {
    appendStringInfoString(str, "edge_id ");
    appendStringInfo(str, "%s %" LWTFMT_ELEMID, op, edge->edge_id);
    sep = sep1;
  }
  if ( fields & LWT_COL_EDGE_START_NODE )
  {
    appendStringInfo(str, "%sstart_node ", sep);
    appendStringInfo(str, "%s %" LWTFMT_ELEMID, op, edge->start_node);
    sep = sep1;
  }
  if ( fields & LWT_COL_EDGE_END_NODE )
  {
    appendStringInfo(str, "%send_node", sep);
    appendStringInfo(str, "%s %" LWTFMT_ELEMID, op, edge->end_node);
    sep = sep1;
  }
  if ( fields & LWT_COL_EDGE_FACE_LEFT )
  {
    appendStringInfo(str, "%sleft_face", sep);
    appendStringInfo(str, "%s %" LWTFMT_ELEMID, op, edge->face_left);
    sep = sep1;
  }
  if ( fields & LWT_COL_EDGE_FACE_RIGHT )
  {
    appendStringInfo(str, "%sright_face", sep);
    appendStringInfo(str, "%s %" LWTFMT_ELEMID, op, edge->face_right);
    sep = sep1;
  }
  if ( fields & LWT_COL_EDGE_NEXT_LEFT )
  {
    appendStringInfo(str, "%snext_left_edge", sep);
    appendStringInfo(str, "%s %" LWTFMT_ELEMID, op, edge->next_left);
    sep = sep1;
    if ( fullEdgeData )
    {
      appendStringInfo(str, "%s abs_next_left_edge", sep);
      appendStringInfo(str, "%s %" LWTFMT_ELEMID, op, ABS(edge->next_left));
    }
  }
  if ( fields & LWT_COL_EDGE_NEXT_RIGHT )
  {
    appendStringInfo(str, "%snext_right_edge", sep);
    appendStringInfo(str, "%s %" LWTFMT_ELEMID, op, edge->next_right);
    sep = sep1;
    if ( fullEdgeData )
    {
      appendStringInfo(str, "%s abs_next_right_edge", sep);
      appendStringInfo(str, "%s %" LWTFMT_ELEMID, op, ABS(edge->next_right));
    }
  }
  if ( fields & LWT_COL_EDGE_GEOM )
  {
    appendStringInfo(str, "%sgeom", sep);
    hexewkb = lwgeom_to_hexwkb_buffer(lwline_as_lwgeom(edge->geom), WKB_EXTENDED);
    appendStringInfo(str, "%s'%s'::geometry", op, hexewkb);
    lwfree(hexewkb);
  }
}

static void
addNodeUpdate(StringInfo str, const LWT_ISO_NODE* node, int fields,
              int fullNodeData, enum UpdateType updType)
{
  const char *sep = "";
  const char *sep1;
  const char *op;
  char *hexewkb;

  switch (updType)
  {
  case updSet:
    op = "=";
    sep1 = ",";
    break;
  case updSel:
    op = "=";
    sep1 = " AND ";
    break;
  case updNot:
  default:
    op = "!=";
    sep1 = " AND ";
    break;
  }

  if ( fields & LWT_COL_NODE_NODE_ID )
  {
    appendStringInfoString(str, "node_id ");
    appendStringInfo(str, "%s %" LWTFMT_ELEMID, op, node->node_id);
    sep = sep1;
  }
  if ( fields & LWT_COL_NODE_CONTAINING_FACE )
  {
    appendStringInfo(str, "%scontaining_face %s", sep, op);
    if ( node->containing_face != -1 )
    {
      appendStringInfo(str, "%" LWTFMT_ELEMID, node->containing_face);
    }
    else
    {
      appendStringInfoString(str, "null::int");
    }
    sep = sep1;
  }
  if ( fields & LWT_COL_NODE_GEOM )
  {
    appendStringInfo(str, "%sgeom", sep);
    hexewkb = lwgeom_to_hexwkb_buffer(lwpoint_as_lwgeom(node->geom), WKB_EXTENDED);
    appendStringInfo(str, "%s'%s'::geometry", op, hexewkb);
    lwfree(hexewkb);
  }
}

static void
addNodeFields(StringInfo str, int fields)
{
  const char *sep = "";

  if ( fields & LWT_COL_NODE_NODE_ID )
  {
    appendStringInfoString(str, "node_id");
    sep = ",";
  }
  if ( fields & LWT_COL_NODE_CONTAINING_FACE )
  {
    appendStringInfo(str, "%scontaining_face", sep);
    sep = ",";
  }
  if ( fields & LWT_COL_NODE_GEOM )
  {
    appendStringInfo(str, "%sgeom", sep);
  }
}

static void
addFaceFields(StringInfo str, int fields)
{
  const char *sep = "";

  if ( fields & LWT_COL_FACE_FACE_ID )
  {
    appendStringInfoString(str, "face_id");
    sep = ",";
  }
  if ( fields & LWT_COL_FACE_MBR )
  {
    appendStringInfo(str, "%smbr", sep);
    sep = ",";
  }
}

/* Add node values for an insert, in text form */
static void
addNodeValues(StringInfo str, const LWT_ISO_NODE *node, int fields)
{
  char *hexewkb;
  const char *sep = "";

  appendStringInfoChar(str, '(');

  if ( fields & LWT_COL_NODE_NODE_ID )
  {
    if ( node->node_id != -1 )
      appendStringInfo(str, "%" LWTFMT_ELEMID, node->node_id);
    else
      appendStringInfoString(str, "DEFAULT");
    sep = ",";
  }

  if ( fields & LWT_COL_NODE_CONTAINING_FACE )
  {
    if ( node->containing_face != -1 )
      appendStringInfo(str, "%s%" LWTFMT_ELEMID, sep, node->containing_face);
    else appendStringInfo(str, "%snull::int", sep);
  }

  if ( fields & LWT_COL_NODE_GEOM )
  {
    if ( node->geom )
    {
	    hexewkb = lwgeom_to_hexwkb_buffer(lwpoint_as_lwgeom(node->geom), WKB_EXTENDED);
	    appendStringInfo(str, "%s'%s'::geometry", sep, hexewkb);
	    lwfree(hexewkb);
    }
    else
    {
      appendStringInfo(str, "%snull::geometry", sep);
    }
  }

  appendStringInfoChar(str, ')');
}

/* Add face values for an insert, in text form */
static void
addFaceValues(StringInfo str, LWT_ISO_FACE *face, int32_t srid)
{
  if ( face->face_id != -1 )
    appendStringInfo(str, "(%" LWTFMT_ELEMID, face->face_id);
  else
    appendStringInfoString(str, "(DEFAULT");

  if ( face->mbr )
  {
    {
      char *hexbox;
      hexbox = _box2d_to_hexwkb(face->mbr, srid);
      appendStringInfo(str, ",ST_Envelope('%s'::geometry))", hexbox);
      lwfree(hexbox);
    }
  }
  else
  {
    appendStringInfoString(str, ",null::geometry)");
  }
}

static void
fillEdgeFields(LWT_ISO_EDGE* edge, HeapTuple row, TupleDesc rowdesc, int fields)
{
  bool isnull;
  Datum dat;
  int val;
  GSERIALIZED *geom;
  LWGEOM *lwg;
  int colno = 0;

  POSTGIS_DEBUGF(2, "fillEdgeFields: got %d atts and fields %x",
                 rowdesc->natts, fields);

  if ( fields & LWT_COL_EDGE_EDGE_ID )
  {
    dat = SPI_getbinval(row, rowdesc, ++colno, &isnull);
    if ( isnull )
    {
      lwpgwarning("Found edge with NULL edge_id");
      edge->edge_id = -1;
    }
    else
    {
      val = DatumGetInt32(dat);
      POSTGIS_DEBUGF(2, "fillEdgeFields: colno%d (edge_id)"
                     " has int32 val of %d",
                     colno, val);
      edge->edge_id = val;
    }

  }
  if ( fields & LWT_COL_EDGE_START_NODE )
  {
    dat = SPI_getbinval(row, rowdesc, ++colno, &isnull);
    if ( isnull )
    {
      lwpgwarning("Found edge with NULL start_node");
      edge->start_node = -1;
    }
    else
    {
      val = DatumGetInt32(dat);
      POSTGIS_DEBUGF(2, "fillEdgeFields: colno%d (start_node)"
                     " has int32 val of %d", colno, val);
      edge->start_node = val;
    }
  }
  if ( fields & LWT_COL_EDGE_END_NODE )
  {
    dat = SPI_getbinval(row, rowdesc, ++colno, &isnull);
    if ( isnull )
    {
      lwpgwarning("Found edge with NULL end_node");
      edge->end_node = -1;
    }
    else
    {
      val = DatumGetInt32(dat);
      POSTGIS_DEBUGF(2, "fillEdgeFields: colno%d (end_node)"
                     " has int32 val of %d", colno, val);
      edge->end_node = val;
    }
  }
  if ( fields & LWT_COL_EDGE_FACE_LEFT )
  {
    dat = SPI_getbinval(row, rowdesc, ++colno, &isnull);
    if ( isnull )
    {
      lwpgwarning("Found edge with NULL face_left");
      edge->face_left = -1;
    }
    else
    {
      val = DatumGetInt32(dat);
      POSTGIS_DEBUGF(2, "fillEdgeFields: colno%d (face_left)"
                     " has int32 val of %d", colno, val);
      edge->face_left = val;
    }
  }
  if ( fields & LWT_COL_EDGE_FACE_RIGHT )
  {
    dat = SPI_getbinval(row, rowdesc, ++colno, &isnull);
    if ( isnull )
    {
      lwpgwarning("Found edge with NULL face_right");
      edge->face_right = -1;
    }
    else
    {
      val = DatumGetInt32(dat);
      POSTGIS_DEBUGF(2, "fillEdgeFields: colno%d (face_right)"
                     " has int32 val of %d", colno, val);
      edge->face_right = val;
    }
  }
  if ( fields & LWT_COL_EDGE_NEXT_LEFT )
  {
    dat = SPI_getbinval(row, rowdesc, ++colno, &isnull);
    if ( isnull )
    {
      lwpgwarning("Found edge with NULL next_left");
      edge->next_left = -1;
    }
    else
    {
      val = DatumGetInt32(dat);
      POSTGIS_DEBUGF(2, "fillEdgeFields: colno%d (next_left)"
                     " has int32 val of %d", colno, val);
      edge->next_left = val;
    }
  }
  if ( fields & LWT_COL_EDGE_NEXT_RIGHT )
  {
    dat = SPI_getbinval(row, rowdesc, ++colno, &isnull);
    if ( isnull )
    {
      lwpgwarning("Found edge with NULL next_right");
      edge->next_right = -1;
    }
    else
    {
      val = DatumGetInt32(dat);
      POSTGIS_DEBUGF(2, "fillEdgeFields: colno%d (next_right)"
                     " has int32 val of %d", colno, val);
      edge->next_right = val;
    }
  }
  if ( fields & LWT_COL_EDGE_GEOM )
  {
    dat = SPI_getbinval(row, rowdesc, ++colno, &isnull);
    if ( ! isnull )
    {
      {
        MemoryContext oldcontext = CurrentMemoryContext;
        geom = (GSERIALIZED *)PG_DETOAST_DATUM(dat);
        lwg = lwgeom_from_gserialized(geom);
        MemoryContextSwitchTo( TopMemoryContext );
        edge->geom = lwgeom_as_lwline(lwgeom_clone_deep(lwg));
        MemoryContextSwitchTo( oldcontext ); /* switch back */
        lwgeom_free(lwg);
        if ( DatumGetPointer(dat) != (Pointer)geom ) pfree(geom); /* IF_COPY */
      }
    }
    else
    {
      lwpgwarning("Found edge with NULL geometry !");
      edge->geom = NULL;
    }
  }
}

static void
fillNodeFields(LWT_ISO_NODE* node, HeapTuple row, TupleDesc rowdesc, int fields)
{
  bool isnull;
  Datum dat;
  GSERIALIZED *geom;
  LWGEOM *lwg;
  int colno = 0;

  if ( fields & LWT_COL_NODE_NODE_ID )
  {
    dat = SPI_getbinval(row, rowdesc, ++colno, &isnull);
    node->node_id = DatumGetInt32(dat);
  }
  if ( fields & LWT_COL_NODE_CONTAINING_FACE )
  {
    dat = SPI_getbinval(row, rowdesc, ++colno, &isnull);
    if ( isnull ) node->containing_face = -1;
    else node->containing_face = DatumGetInt32(dat);
  }
  if ( fields & LWT_COL_NODE_GEOM )
  {
    dat = SPI_getbinval(row, rowdesc, ++colno, &isnull);
    if ( ! isnull )
    {
      geom = (GSERIALIZED *)PG_DETOAST_DATUM(dat);
      lwg = lwgeom_from_gserialized(geom);
      node->geom = lwgeom_as_lwpoint(lwgeom_clone_deep(lwg));
      lwgeom_free(lwg);
      if ( DatumGetPointer(dat) != (Pointer)geom ) pfree(geom); /* IF_COPY */
    }
    else
    {
      lwpgnotice("Found node with NULL geometry !");
      node->geom = NULL;
    }
  }
}

static void
fillFaceFields(LWT_ISO_FACE* face, HeapTuple row, TupleDesc rowdesc, int fields)
{
  bool isnull;
  Datum dat;
  GSERIALIZED *geom;
  LWGEOM *g;
  const GBOX *box;
  int colno = 0;

  if ( fields & LWT_COL_FACE_FACE_ID )
  {
    dat = SPI_getbinval(row, rowdesc, ++colno, &isnull);
    face->face_id = DatumGetInt32(dat);
  }
  if ( fields & LWT_COL_FACE_MBR )
  {
    dat = SPI_getbinval(row, rowdesc, ++colno, &isnull);
    if ( ! isnull )
    {
      /* NOTE: this is a geometry of which we want to take (and clone) the BBOX */
      geom = (GSERIALIZED *)PG_DETOAST_DATUM(dat);
      g = lwgeom_from_gserialized(geom);
      box = lwgeom_get_bbox(g);
      if ( box )
      {
        face->mbr = gbox_clone(box);
      }
      else
      {
        lwpgnotice("Found face with EMPTY MBR !");
        face->mbr = NULL;
      }
      lwgeom_free(g);
      if ( DatumGetPointer(dat) != (Pointer)geom ) pfree(geom);
    }
    else
    {
      /* NOTE: perfectly fine for universe face */
      POSTGIS_DEBUG(1, "Found face with NULL MBR");
      face->mbr = NULL;
    }
  }
}

/* return 0 on failure (null) 1 otherwise */
static int
getNotNullInt32( HeapTuple row, TupleDesc desc, int col, int32 *val )
{
  bool isnull;
  Datum dat = SPI_getbinval( row, desc, col, &isnull );
  if ( isnull ) return 0;
  *val = DatumGetInt32(dat);
  return 1;
}

/* ----------------- Callbacks start here ------------------------ */

static LWT_ISO_EDGE *
cb_getEdgeById(const LWT_BE_TOPOLOGY *topo, const LWT_ELEMID *ids, uint64_t *numelems, int fields)
{
  LWT_ISO_EDGE *edges;
  int spi_result;
  MemoryContext oldcontext = CurrentMemoryContext;
  StringInfoData sqldata;
  StringInfo sql = &sqldata;
  uint64_t i;

  initStringInfo(sql);
  appendStringInfoString(sql, "SELECT ");
  addEdgeFields(sql, fields, 0);
  appendStringInfo(sql, " FROM \"%s\".edge_data", topo->name);
  appendStringInfoString(sql, " WHERE edge_id IN (");
  // add all identifiers here
  for (i=0; i<*numelems; ++i)
  {
    appendStringInfo(sql, "%s%" LWTFMT_ELEMID, (i?",":""), ids[i]);
  }
  appendStringInfoString(sql, ")");
  POSTGIS_DEBUGF(1, "cb_getEdgeById query: %s", sql->data);

  spi_result = SPI_execute(sql->data, !topo->be_data->data_changed, *numelems);
  MemoryContextSwitchTo( oldcontext ); /* switch back */
  if ( spi_result != SPI_OK_SELECT )
  {
    cberror(topo->be_data, "unexpected return (%d) from query execution: %s", spi_result, sql->data);
    pfree(sqldata.data);
    *numelems = UINT64_MAX;
    return NULL;
  }
  pfree(sqldata.data);

  POSTGIS_DEBUGF(1, "cb_getEdgeById: edge query returned " UINT64_FORMAT " rows", SPI_processed);
  *numelems = SPI_processed;
  if ( ! SPI_processed )
  {
    return NULL;
  }

  edges = palloc( sizeof(LWT_ISO_EDGE) * *numelems );
  for ( i=0; i<*numelems; ++i )
  {
    HeapTuple row = SPI_tuptable->vals[i];
    fillEdgeFields(&edges[i], row, SPI_tuptable->tupdesc, fields);
  }

  SPI_freetuptable(SPI_tuptable);

  return edges;
}

static LWT_ISO_EDGE *
cb_getEdgeByNode(const LWT_BE_TOPOLOGY *topo, const LWT_ELEMID *ids, uint64_t *numelems, int fields)
{
  LWT_ISO_EDGE *edges;
  int spi_result;

  StringInfoData sqldata;
  StringInfo sql = &sqldata;
  uint64_t i;
  MemoryContext oldcontext = CurrentMemoryContext;

  initStringInfo(sql);
  appendStringInfoString(sql, "SELECT ");
  addEdgeFields(sql, fields, 0);
  appendStringInfo(sql, " FROM \"%s\".edge_data", topo->name);
  appendStringInfoString(sql, " WHERE start_node IN (");
  // add all identifiers here
  for (i=0; i<*numelems; ++i)
  {
    appendStringInfo(sql, "%s%" LWTFMT_ELEMID, (i?",":""), ids[i]);
  }
  appendStringInfoString(sql, ") OR end_node IN (");
  // add all identifiers here
  for (i=0; i<*numelems; ++i)
  {
    appendStringInfo(sql, "%s%" LWTFMT_ELEMID, (i?",":""), ids[i]);
  }
  appendStringInfoString(sql, ")");

  POSTGIS_DEBUGF(1, "cb_getEdgeByNode query: %s", sql->data);
  POSTGIS_DEBUGF(1, "data_changed is %d", topo->be_data->data_changed);

  spi_result = SPI_execute(sql->data, !topo->be_data->data_changed, 0);
  MemoryContextSwitchTo( oldcontext ); /* switch back */
  if ( spi_result != SPI_OK_SELECT )
  {
    cberror(topo->be_data, "unexpected return (%d) from query execution: %s", spi_result, sql->data);
    pfree(sqldata.data);
    *numelems = UINT64_MAX;
    return NULL;
  }
  pfree(sqldata.data);

  POSTGIS_DEBUGF(1, "cb_getEdgeByNode: edge query returned " UINT64_FORMAT " rows", SPI_processed);
  *numelems = SPI_processed;
  if ( ! SPI_processed )
  {
    return NULL;
  }

  edges = palloc( sizeof(LWT_ISO_EDGE) * *numelems );
  for ( i=0; i<*numelems; ++i )
  {
    HeapTuple row = SPI_tuptable->vals[i];
    fillEdgeFields(&edges[i], row, SPI_tuptable->tupdesc, fields);
  }

  SPI_freetuptable(SPI_tuptable);

  return edges;
}

static LWT_ISO_EDGE *
cb_getEdgeByFace(const LWT_BE_TOPOLOGY *topo, const LWT_ELEMID *ids, uint64_t *numelems, int fields, const GBOX *box)
{
  LWT_ISO_EDGE *edges;
  int spi_result;
  MemoryContext oldcontext = CurrentMemoryContext;
  StringInfoData sqldata;
  StringInfo sql = &sqldata;
  uint64_t i;
  ArrayType *array_ids;
  Datum *datum_ids;
  Datum values[2];
  Oid argtypes[2];
  int nargs = 1;
  GSERIALIZED *gser = NULL;

  datum_ids = palloc(sizeof(Datum)*(*numelems));
  for (i=0; i<*numelems; ++i) datum_ids[i] = Int32GetDatum(ids[i]);
  array_ids = construct_array(datum_ids, *numelems, INT4OID, 4, true, 's');

  initStringInfo(sql);
  appendStringInfoString(sql, "SELECT ");
  addEdgeFields(sql, fields, 0);
  appendStringInfo(sql, " FROM \"%s\".edge_data"
                   " WHERE ( left_face = ANY($1) "
                   " OR right_face = ANY ($1) )",
                   topo->name);

  values[0] = PointerGetDatum(array_ids);
  argtypes[0] = INT4ARRAYOID;

  if ( box )
  {
    LWGEOM *g = _box2d_to_lwgeom(box, topo->srid);
    gser = geometry_serialize(g);
    lwgeom_free(g);
    appendStringInfo(sql, " AND geom && $2");

    values[1] = PointerGetDatum(gser);
    argtypes[1] = topo->geometryOID;
    ++nargs;
  }

  POSTGIS_DEBUGF(1, "cb_getEdgeByFace query: %s", sql->data);
  POSTGIS_DEBUGF(1, "data_changed is %d", topo->be_data->data_changed);

  spi_result = SPI_execute_with_args(sql->data, nargs, argtypes, values, NULL,
                                     !topo->be_data->data_changed, 0);
  pfree(array_ids); /* not needed anymore */
  if ( gser ) pfree(gser); /* not needed anymore */
  MemoryContextSwitchTo( oldcontext ); /* switch back */
  if ( spi_result != SPI_OK_SELECT )
  {
    cberror(topo->be_data, "unexpected return (%d) from query execution: %s", spi_result, sql->data);
    pfree(sqldata.data);
    *numelems = UINT64_MAX;
    return NULL;
  }
  pfree(sqldata.data);

  POSTGIS_DEBUGF(1, "cb_getEdgeByFace: edge query returned " UINT64_FORMAT " rows", SPI_processed);
  *numelems = SPI_processed;
  if ( ! SPI_processed )
  {
    return NULL;
  }

  edges = palloc( sizeof(LWT_ISO_EDGE) * *numelems );
  for ( i=0; i<*numelems; ++i )
  {
    HeapTuple row = SPI_tuptable->vals[i];
    fillEdgeFields(&edges[i], row, SPI_tuptable->tupdesc, fields);
  }

  SPI_freetuptable(SPI_tuptable);

  return edges;
}

static LWT_ISO_FACE *
cb_getFacesById(const LWT_BE_TOPOLOGY *topo, const LWT_ELEMID *ids, uint64_t *numelems, int fields)
{
  LWT_ISO_FACE *faces;
  int spi_result;
  StringInfoData sqldata;
  StringInfo sql = &sqldata;
  uint64_t i;
  MemoryContext oldcontext = CurrentMemoryContext;

  initStringInfo(sql);
  appendStringInfoString(sql, "SELECT ");
  addFaceFields(sql, fields);
  appendStringInfo(sql, " FROM \"%s\".face", topo->name);
  appendStringInfoString(sql, " WHERE face_id IN (");
  // add all identifiers here
  for (i=0; i<*numelems; ++i)
  {
    appendStringInfo(sql, "%s%" LWTFMT_ELEMID, (i?",":""), ids[i]);
  }
  appendStringInfoString(sql, ")");

  POSTGIS_DEBUGF(1, "cb_getFaceById query: %s", sql->data);
  POSTGIS_DEBUGF(1, "data_changed is %d", topo->be_data->data_changed);

  spi_result = SPI_execute(sql->data, !topo->be_data->data_changed, 0);
  MemoryContextSwitchTo( oldcontext ); /* switch back */
  if ( spi_result != SPI_OK_SELECT )
  {
    cberror(topo->be_data, "unexpected return (%d) from query execution: %s", spi_result, sql->data);
    pfree(sqldata.data);
    *numelems = UINT64_MAX;
    return NULL;
  }
  pfree(sqldata.data);

  POSTGIS_DEBUGF(1, "cb_getFaceById: face query returned " UINT64_FORMAT " rows", SPI_processed);
  *numelems = SPI_processed;
  if ( ! SPI_processed )
  {
    return NULL;
  }

  faces = palloc( sizeof(LWT_ISO_EDGE) * *numelems );
  for ( i=0; i<*numelems; ++i )
  {
    HeapTuple row = SPI_tuptable->vals[i];
    fillFaceFields(&faces[i], row, SPI_tuptable->tupdesc, fields);
  }

  SPI_freetuptable(SPI_tuptable);

  return faces;
}

static LWT_ELEMID *
cb_getRingEdges(const LWT_BE_TOPOLOGY *topo, LWT_ELEMID edge, uint64_t *numelems, int limit)
{
  LWT_ELEMID *edges;
  int spi_result;
  TupleDesc rowdesc;
  StringInfoData sqldata;
  StringInfo sql = &sqldata;
  uint64_t i;
  MemoryContext oldcontext = CurrentMemoryContext;

  initStringInfo(sql);
  appendStringInfo(sql, "WITH RECURSIVE edgering AS ( "
                   "SELECT %" LWTFMT_ELEMID
                   " as signed_edge_id, edge_id, next_left_edge, next_right_edge "
                   "FROM \"%s\".edge_data WHERE edge_id = %" LWTFMT_ELEMID " UNION "
                   "SELECT CASE WHEN "
                   "p.signed_edge_id < 0 THEN p.next_right_edge ELSE p.next_left_edge END, "
                   "e.edge_id, e.next_left_edge, e.next_right_edge "
                   "FROM \"%s\".edge_data e, edgering p WHERE "
                   "e.edge_id = CASE WHEN p.signed_edge_id < 0 THEN "
                   "abs(p.next_right_edge) ELSE abs(p.next_left_edge) END ) "
                   "SELECT * FROM edgering",
                   edge, topo->name, ABS(edge), topo->name);
  if ( limit )
  {
    ++limit; /* so we know if we hit it */
    appendStringInfo(sql, " LIMIT %d", limit);
  }

  POSTGIS_DEBUGF(1, "cb_getRingEdges query (limit %d): %s", limit, sql->data);
  spi_result = SPI_execute(sql->data, !topo->be_data->data_changed, limit);
  MemoryContextSwitchTo( oldcontext ); /* switch back */
  if ( spi_result != SPI_OK_SELECT )
  {
    cberror(topo->be_data, "unexpected return (%d) from query execution: %s", spi_result, sql->data);
    pfree(sqldata.data);
    *numelems = UINT64_MAX;
    return NULL;
  }
  pfree(sqldata.data);

  POSTGIS_DEBUGF(1, "cb_getRingEdges: edge query returned " UINT64_FORMAT " rows", SPI_processed);
  *numelems = SPI_processed;
  if ( ! SPI_processed )
  {
    return NULL;
  }
  if (limit && *numelems == (uint64_t)limit)
  {
	  cberror(topo->be_data, "Max traversing limit hit: %d", limit - 1);
	  *numelems = UINT64_MAX;
	  return NULL;
  }

  edges = palloc( sizeof(LWT_ELEMID) * *numelems );
  rowdesc = SPI_tuptable->tupdesc;
  for ( i=0; i<*numelems; ++i )
  {
    HeapTuple row = SPI_tuptable->vals[i];
    bool isnull;
    Datum dat;
    int32 val;
    dat = SPI_getbinval(row, rowdesc, 1, &isnull);
    if ( isnull )
    {
      lwfree(edges);
      cberror(topo->be_data, "Found edge with NULL edge_id");
      *numelems = UINT64_MAX;
      return NULL;
    }
    val = DatumGetInt32(dat);
    edges[i] = val;
    POSTGIS_DEBUGF(1, "Component " UINT64_FORMAT " in ring of edge %" LWTFMT_ELEMID " is edge %d", i, edge, val);

    /* For the last entry, check that we returned back to start
     * point, or complain about topology being corrupted */
    if ( i == *numelems - 1 )
    {
      int32 nextedge;
      int sidecol = val > 0 ? 3 : 4;
      const char *sidetext = val > 0 ? "left" : "right";

      dat = SPI_getbinval(row, rowdesc, sidecol, &isnull);
      if ( isnull )
      {
        lwfree(edges);
        cberror(topo->be_data, "Edge %d" /*LWTFMT_ELEMID*/
                               " has NULL next_%s_edge",
                               val, sidetext);
        *numelems = UINT64_MAX;
        return NULL;
      }
      nextedge = DatumGetInt32(dat);
      POSTGIS_DEBUGF(1, "Last component in ring of edge %"
                        LWTFMT_ELEMID " (%" LWTFMT_ELEMID ") has next_%s_edge %d",
                        edge, val, sidetext, nextedge);
      if ( nextedge != edge )
      {
        lwfree(edges);
        cberror(topo->be_data, "Corrupted topology: ring of edge %"
                               LWTFMT_ELEMID " is topologically non-closed",
                               edge);
        *numelems = UINT64_MAX;
        return NULL;
      }
    }

  }

  SPI_freetuptable(SPI_tuptable);

  return edges;
}

static LWT_ISO_NODE *
cb_getNodeById(const LWT_BE_TOPOLOGY *topo, const LWT_ELEMID *ids, uint64_t *numelems, int fields)
{
  LWT_ISO_NODE *nodes;
  int spi_result;

  StringInfoData sqldata;
  StringInfo sql = &sqldata;
  uint64_t i;
  MemoryContext oldcontext = CurrentMemoryContext;

  initStringInfo(sql);
  appendStringInfoString(sql, "SELECT ");
  addNodeFields(sql, fields);
  appendStringInfo(sql, " FROM \"%s\".node", topo->name);
  appendStringInfoString(sql, " WHERE node_id IN (");
  // add all identifiers here
  for (i=0; i<*numelems; ++i)
  {
    appendStringInfo(sql, "%s%" LWTFMT_ELEMID, (i?",":""), ids[i]);
  }
  appendStringInfoString(sql, ")");
  POSTGIS_DEBUGF(1, "cb_getNodeById query: %s", sql->data);
  spi_result = SPI_execute(sql->data, !topo->be_data->data_changed, *numelems);
  MemoryContextSwitchTo( oldcontext ); /* switch back */
  if ( spi_result != SPI_OK_SELECT )
  {
    cberror(topo->be_data, "unexpected return (%d) from query execution: %s", spi_result, sql->data);
    pfree(sqldata.data);
    *numelems = -1;
    return NULL;
  }
  pfree(sqldata.data);

  POSTGIS_DEBUGF(1, "cb_getNodeById: edge query returned " UINT64_FORMAT " rows", SPI_processed);
  *numelems = SPI_processed;
  if ( ! SPI_processed )
  {
    return NULL;
  }

  nodes = palloc( sizeof(LWT_ISO_NODE) * *numelems );
  for ( i=0; i<*numelems; ++i )
  {
    HeapTuple row = SPI_tuptable->vals[i];
    fillNodeFields(&nodes[i], row, SPI_tuptable->tupdesc, fields);
  }

  SPI_freetuptable(SPI_tuptable);

  return nodes;
}

static LWT_ISO_NODE *
cb_getNodeByFace(const LWT_BE_TOPOLOGY *topo, const LWT_ELEMID *ids, uint64_t *numelems, int fields, const GBOX *box)
{
  LWT_ISO_NODE *nodes;
  int spi_result;
  MemoryContext oldcontext = CurrentMemoryContext;
  StringInfoData sqldata;
  StringInfo sql = &sqldata;
  uint64_t i;
  char *hexbox;

  initStringInfo(sql);
  appendStringInfoString(sql, "SELECT ");
  addNodeFields(sql, fields);
  appendStringInfo(sql, " FROM \"%s\".node", topo->name);
  appendStringInfoString(sql, " WHERE containing_face IN (");
  // add all identifiers here
  for (i=0; i<*numelems; ++i)
  {
    appendStringInfo(sql, "%s%" LWTFMT_ELEMID, (i?",":""), ids[i]);
  }
  appendStringInfoString(sql, ")");
  if ( box )
  {
    hexbox = _box2d_to_hexwkb(box, topo->srid);
    appendStringInfo(sql, " AND geom && '%s'::geometry", hexbox);
    lwfree(hexbox);
  }
  POSTGIS_DEBUGF(1, "cb_getNodeByFace query: %s", sql->data);
  POSTGIS_DEBUGF(1, "data_changed is %d", topo->be_data->data_changed);
  spi_result = SPI_execute(sql->data, !topo->be_data->data_changed, 0);
  MemoryContextSwitchTo( oldcontext ); /* switch back */
  if ( spi_result != SPI_OK_SELECT )
  {
    cberror(topo->be_data, "unexpected return (%d) from query execution: %s", spi_result, sql->data);
    pfree(sqldata.data);
    *numelems = UINT64_MAX;
    return NULL;
  }
  pfree(sqldata.data);

  POSTGIS_DEBUGF(1, "cb_getNodeByFace: edge query returned " UINT64_FORMAT " rows", SPI_processed);
  *numelems = SPI_processed;
  if ( ! SPI_processed )
  {
    return NULL;
  }

  nodes = palloc( sizeof(LWT_ISO_NODE) * *numelems );
  for ( i=0; i<*numelems; ++i )
  {
    HeapTuple row = SPI_tuptable->vals[i];
    fillNodeFields(&nodes[i], row, SPI_tuptable->tupdesc, fields);
  }

  SPI_freetuptable(SPI_tuptable);

  return nodes;
}

static LWT_ISO_EDGE *
cb_getEdgeWithinDistance2D(const LWT_BE_TOPOLOGY *topo,
			   const LWPOINT *pt,
			   double dist,
			   uint64_t *numelems,
			   int fields,
			   int64_t limit)
{
  LWT_ISO_EDGE *edges;
  int spi_result;
  int64_t elems_requested = limit;
  char *hexewkb;
  MemoryContext oldcontext = CurrentMemoryContext;
  StringInfoData sqldata;
  StringInfo sql = &sqldata;
  uint64_t i;

  initStringInfo(sql);
  if ( elems_requested == -1 )
  {
    appendStringInfoString(sql, "SELECT EXISTS ( SELECT 1");
  }
  else
  {
    appendStringInfoString(sql, "SELECT ");
    addEdgeFields(sql, fields, 0);
  }
  appendStringInfo(sql, " FROM \"%s\".edge_data", topo->name);
  // TODO: use binary cursor here ?
  hexewkb = lwgeom_to_hexwkb_buffer(lwpoint_as_lwgeom(pt), WKB_EXTENDED);
  if ( dist )
  {
    appendStringInfo(sql, " WHERE ST_DWithin('%s'::geometry, geom, %g)", hexewkb, dist);
  }
  else
  {
    appendStringInfo(sql, " WHERE ST_Within('%s'::geometry, geom)", hexewkb);
  }
  lwfree(hexewkb);
  if ( elems_requested == -1 )
  {
    appendStringInfoString(sql, ")");
  }
  else if ( elems_requested > 0 )
  {
	  appendStringInfo(sql, " LIMIT " INT64_FORMAT, elems_requested);
  }
  POSTGIS_DEBUGF(1, "cb_getEdgeWithinDistance2D: query is: %s", sql->data);
  spi_result = SPI_execute(sql->data, !topo->be_data->data_changed, limit >= 0 ? limit : 0);
  MemoryContextSwitchTo( oldcontext ); /* switch back */
  if ( spi_result != SPI_OK_SELECT )
  {
    cberror(topo->be_data, "unexpected return (%d) from query execution: %s", spi_result, sql->data);
    pfree(sqldata.data);
    *numelems = UINT64_MAX;
    return NULL;
  }
  pfree(sqldata.data);

  POSTGIS_DEBUGF(1,
		 "cb_getEdgeWithinDistance2D: edge query "
		 "(limited by " INT64_FORMAT ") returned " UINT64_FORMAT " rows",
		 elems_requested,
		 SPI_processed);
  *numelems = SPI_processed;
  if ( ! SPI_processed )
  {
    return NULL;
  }

  if ( elems_requested == -1 )
  {
    /* This was an EXISTS query */
    {
      Datum dat;
      bool isnull, exists;
      dat = SPI_getbinval(SPI_tuptable->vals[0], SPI_tuptable->tupdesc, 1, &isnull);
      exists = DatumGetBool(dat);
      *numelems = exists ? 1 : 0;
      POSTGIS_DEBUGF(1, "cb_getEdgeWithinDistance2D: exists ? " UINT64_FORMAT, *numelems);
    }

    SPI_freetuptable(SPI_tuptable);

    return NULL;
  }

  edges = palloc( sizeof(LWT_ISO_EDGE) * *numelems );
  for ( i=0; i<*numelems; ++i )
  {
    HeapTuple row = SPI_tuptable->vals[i];
    fillEdgeFields(&edges[i], row, SPI_tuptable->tupdesc, fields);
  }

  SPI_freetuptable(SPI_tuptable);

  return edges;
}

static LWT_ISO_NODE *
cb_getNodeWithinDistance2D(const LWT_BE_TOPOLOGY *topo,
			   const LWPOINT *pt,
			   double dist,
			   uint64_t *numelems,
			   int fields,
			   int64_t limit)
{
  MemoryContext oldcontext = CurrentMemoryContext;
  LWT_ISO_NODE *nodes;
  int spi_result;
  char *hexewkb;
  StringInfoData sqldata;
  StringInfo sql = &sqldata;
  int64_t elems_requested = limit;
  uint64_t i;

  initStringInfo(sql);
  if ( elems_requested == -1 )
  {
    appendStringInfoString(sql, "SELECT EXISTS ( SELECT 1");
  }
  else
  {
    appendStringInfoString(sql, "SELECT ");
    if ( fields ) addNodeFields(sql, fields);
    else
    {
      lwpgwarning("liblwgeom-topo invoked 'getNodeWithinDistance2D' "
                  "backend callback with limit=%d and no fields",
                  elems_requested);
      appendStringInfo(sql, "*");
    }
  }
  appendStringInfo(sql, " FROM \"%s\".node", topo->name);
  // TODO: use binary cursor here ?
  hexewkb = lwgeom_to_hexwkb_buffer(lwpoint_as_lwgeom(pt), WKB_EXTENDED);
  if ( dist )
  {
    appendStringInfo(sql, " WHERE ST_DWithin(geom, '%s'::geometry, %g)",
                     hexewkb, dist);
  }
  else
  {
    appendStringInfo(sql, " WHERE ST_Equals(geom, '%s'::geometry)", hexewkb);
  }
  lwfree(hexewkb);
  if ( elems_requested == -1 )
  {
    appendStringInfoString(sql, ")");
  }
  else if ( elems_requested > 0 )
  {
	  appendStringInfo(sql, " LIMIT " INT64_FORMAT, elems_requested);
  }
  spi_result = SPI_execute(sql->data, !topo->be_data->data_changed, limit >= 0 ? limit : 0);
  MemoryContextSwitchTo( oldcontext ); /* switch back */
  if ( spi_result != SPI_OK_SELECT )
  {
    cberror(topo->be_data, "unexpected return (%d) from query execution: %s",
            spi_result, sql->data);
    pfree(sqldata.data);
    *numelems = UINT64_MAX;
    return NULL;
  }
  pfree(sqldata.data);

  POSTGIS_DEBUGF(1,
		 "cb_getNodeWithinDistance2D: node query "
		 "(limited by " INT64_FORMAT ") returned " UINT64_FORMAT " rows",
		 elems_requested,
		 SPI_processed);
  if ( ! SPI_processed )
  {
    *numelems = 0;
    return NULL;
  }

  if ( elems_requested == -1 )
  {
    /* This was an EXISTS query */
    {
      Datum dat;
      bool isnull, exists;
      dat = SPI_getbinval(SPI_tuptable->vals[0], SPI_tuptable->tupdesc, 1, &isnull);
      exists = DatumGetBool(dat);
      *numelems = exists ? 1 : 0;
    }

    SPI_freetuptable(SPI_tuptable);

    return NULL;
  }
  else
  {
    *numelems = SPI_processed;
    nodes = palloc( sizeof(LWT_ISO_EDGE) * *numelems );
    for ( i=0; i<*numelems; ++i )
    {
      HeapTuple row = SPI_tuptable->vals[i];
      fillNodeFields(&nodes[i], row, SPI_tuptable->tupdesc, fields);
    }

    SPI_freetuptable(SPI_tuptable);

    return nodes;
  }
}

static int
cb_insertNodes(const LWT_BE_TOPOLOGY *topo, LWT_ISO_NODE *nodes, uint64_t numelems)
{
  MemoryContext oldcontext = CurrentMemoryContext;
  int spi_result;
  StringInfoData sqldata;
  StringInfo sql = &sqldata;
  uint64_t i;

  initStringInfo(sql);
  appendStringInfo(sql, "INSERT INTO \"%s\".node (", topo->name);
  addNodeFields(sql, LWT_COL_NODE_ALL);
  appendStringInfoString(sql, ") VALUES ");
  for ( i=0; i<numelems; ++i )
  {
    if ( i ) appendStringInfoString(sql, ",");
    // TODO: prepare and execute ?
    addNodeValues(sql, &nodes[i], LWT_COL_NODE_ALL);
  }
  appendStringInfoString(sql, " RETURNING node_id");

  POSTGIS_DEBUGF(1, "cb_insertNodes query: %s", sql->data);

  spi_result = SPI_execute(sql->data, false, numelems);
  MemoryContextSwitchTo( oldcontext ); /* switch back */
  if ( spi_result != SPI_OK_INSERT_RETURNING )
  {
    cberror(topo->be_data, "unexpected return (%d) from query execution: %s",
            spi_result, sql->data);
    pfree(sqldata.data);
    return 0;
  }
  pfree(sqldata.data);

  if ( SPI_processed ) topo->be_data->data_changed = true;

  if (SPI_processed != numelems)
  {
	  cberror(topo->be_data,
		  "processed " UINT64_FORMAT " rows, expected " UINT64_FORMAT,
		  (uint64_t)SPI_processed,
		  numelems);
	  return 0;
  }

  /* Set node_id (could skip this if none had it set to -1) */
  /* TODO: check for -1 values in the first loop */
  for ( i=0; i<numelems; ++i )
  {
    if ( nodes[i].node_id != -1 ) continue;
    fillNodeFields(&nodes[i], SPI_tuptable->vals[i],
                   SPI_tuptable->tupdesc, LWT_COL_NODE_NODE_ID);
  }

  SPI_freetuptable(SPI_tuptable);

  return 1;
}

static int
cb_insertEdges(const LWT_BE_TOPOLOGY *topo, LWT_ISO_EDGE *edges, uint64_t numelems)
{
  MemoryContext oldcontext = CurrentMemoryContext;
  int spi_result;
  StringInfoData sqldata;
  StringInfo sql = &sqldata;
  uint64_t i;
  int needsEdgeIdReturn = 0;

  initStringInfo(sql);
  /* NOTE: we insert into "edge", on which an insert rule is defined */
  appendStringInfo(sql, "INSERT INTO \"%s\".edge_data (", topo->name);
  addEdgeFields(sql, LWT_COL_EDGE_ALL, 1);
  appendStringInfoString(sql, ") VALUES ");
  for ( i=0; i<numelems; ++i )
  {
    if ( i ) appendStringInfoString(sql, ",");
    // TODO: prepare and execute ?
    addEdgeValues(sql, &edges[i], LWT_COL_EDGE_ALL, 1);
    if ( edges[i].edge_id == -1 ) needsEdgeIdReturn = 1;
  }
  if ( needsEdgeIdReturn ) appendStringInfoString(sql, " RETURNING edge_id");

  POSTGIS_DEBUGF(1, "cb_insertEdges query (" UINT64_FORMAT " elems): %s", numelems, sql->data);
  spi_result = SPI_execute(sql->data, false, numelems);
  MemoryContextSwitchTo( oldcontext ); /* switch back */
  if ( spi_result != ( needsEdgeIdReturn ? SPI_OK_INSERT_RETURNING : SPI_OK_INSERT ) )
  {
    cberror(topo->be_data, "unexpected return (%d) from query execution: %s",
            spi_result, sql->data);
    pfree(sqldata.data);
    return -1;
  }
  pfree(sqldata.data);
  if ( SPI_processed ) topo->be_data->data_changed = true;
  POSTGIS_DEBUGF(1, "cb_insertEdges query processed " UINT64_FORMAT " rows", SPI_processed);
  if ( SPI_processed != (uint64) numelems )
  {
	  cberror(topo->be_data,
		  "processed " UINT64_FORMAT " rows, expected " UINT64_FORMAT,
		  (uint64_t)SPI_processed,
		  numelems);
	  return -1;
  }

  if ( needsEdgeIdReturn )
  {
    /* Set node_id for items that need it */
    for (i = 0; i < SPI_processed; ++i)
    {
      if ( edges[i].edge_id != -1 ) continue;
      fillEdgeFields(&edges[i], SPI_tuptable->vals[i],
                     SPI_tuptable->tupdesc, LWT_COL_EDGE_EDGE_ID);
    }
  }

  SPI_freetuptable(SPI_tuptable);

  return SPI_processed;
}

static int
cb_insertFaces(const LWT_BE_TOPOLOGY *topo, LWT_ISO_FACE *faces, uint64_t numelems)
{
  MemoryContext oldcontext = CurrentMemoryContext;
  int spi_result;
  StringInfoData sqldata;
  StringInfo sql = &sqldata;
  uint64_t i;
  int needsFaceIdReturn = 0;

  initStringInfo(sql);
  appendStringInfo(sql, "INSERT INTO \"%s\".face (", topo->name);
  addFaceFields(sql, LWT_COL_FACE_ALL);
  appendStringInfoString(sql, ") VALUES ");
  for ( i=0; i<numelems; ++i )
  {
    if ( i ) appendStringInfoString(sql, ",");
    // TODO: prepare and execute ?
    addFaceValues(sql, &faces[i], topo->srid);
    if ( faces[i].face_id == -1 ) needsFaceIdReturn = 1;
  }
  if ( needsFaceIdReturn ) appendStringInfoString(sql, " RETURNING face_id");

  POSTGIS_DEBUGF(1, "cb_insertFaces query (" UINT64_FORMAT " elems): %s", numelems, sql->data);
  spi_result = SPI_execute(sql->data, false, numelems);
  MemoryContextSwitchTo( oldcontext ); /* switch back */
  if ( spi_result != ( needsFaceIdReturn ? SPI_OK_INSERT_RETURNING : SPI_OK_INSERT ) )
  {
    cberror(topo->be_data, "unexpected return (%d) from query execution: %s",
            spi_result, sql->data);
    pfree(sqldata.data);
    return -1;
  }
  pfree(sqldata.data);
  if ( SPI_processed ) topo->be_data->data_changed = true;
  POSTGIS_DEBUGF(1, "cb_insertFaces query processed " UINT64_FORMAT " rows", SPI_processed);
  if (SPI_processed != numelems)
  {
	  cberror(topo->be_data,
		  "processed " UINT64_FORMAT " rows, expected " UINT64_FORMAT,
		  (uint64_t)SPI_processed,
		  numelems);
	  return -1;
  }

  if ( needsFaceIdReturn )
  {
    /* Set node_id for items that need it */
    for ( i=0; i<numelems; ++i )
    {
      if ( faces[i].face_id != -1 ) continue;
      fillFaceFields(&faces[i], SPI_tuptable->vals[i],
                     SPI_tuptable->tupdesc, LWT_COL_FACE_FACE_ID);
    }
  }

  SPI_freetuptable(SPI_tuptable);

  return SPI_processed;
}

static int
cb_updateEdges( const LWT_BE_TOPOLOGY* topo,
                const LWT_ISO_EDGE* sel_edge, int sel_fields,
                const LWT_ISO_EDGE* upd_edge, int upd_fields,
                const LWT_ISO_EDGE* exc_edge, int exc_fields )
{
  MemoryContext oldcontext = CurrentMemoryContext;
  int spi_result;
  StringInfoData sqldata;
  StringInfo sql = &sqldata;

  initStringInfo(sql);
  appendStringInfo(sql, "UPDATE \"%s\".edge_data SET ", topo->name);
  addEdgeUpdate( sql, upd_edge, upd_fields, 1, updSet );
  if ( exc_edge || sel_edge ) appendStringInfoString(sql, " WHERE ");
  if ( sel_edge )
  {
    addEdgeUpdate( sql, sel_edge, sel_fields, 1, updSel );
    if ( exc_edge ) appendStringInfoString(sql, " AND ");
  }
  if ( exc_edge )
  {
    addEdgeUpdate( sql, exc_edge, exc_fields, 1, updNot );
  }

  POSTGIS_DEBUGF(1, "cb_updateEdges query: %s", sql->data);

  spi_result = SPI_execute( sql->data, false, 0 );
  MemoryContextSwitchTo( oldcontext ); /* switch back */
  if ( spi_result != SPI_OK_UPDATE )
  {
    cberror(topo->be_data, "unexpected return (%d) from query execution: %s",
            spi_result, sql->data);
    pfree(sqldata.data);
    return -1;
  }
  pfree(sqldata.data);

  if ( SPI_processed ) topo->be_data->data_changed = true;

  POSTGIS_DEBUGF(1, "cb_updateEdges: update query processed " UINT64_FORMAT " rows", SPI_processed);

  return SPI_processed;
}

static int
cb_updateNodes( const LWT_BE_TOPOLOGY* topo,
                const LWT_ISO_NODE* sel_node, int sel_fields,
                const LWT_ISO_NODE* upd_node, int upd_fields,
                const LWT_ISO_NODE* exc_node, int exc_fields )
{
  MemoryContext oldcontext = CurrentMemoryContext;
  int spi_result;
  StringInfoData sqldata;
  StringInfo sql = &sqldata;

  initStringInfo(sql);
  appendStringInfo(sql, "UPDATE \"%s\".node SET ", topo->name);
  addNodeUpdate( sql, upd_node, upd_fields, 1, updSet );
  if ( exc_node || sel_node ) appendStringInfoString(sql, " WHERE ");
  if ( sel_node )
  {
    addNodeUpdate( sql, sel_node, sel_fields, 1, updSel );
    if ( exc_node ) appendStringInfoString(sql, " AND ");
  }
  if ( exc_node )
  {
    addNodeUpdate( sql, exc_node, exc_fields, 1, updNot );
  }

  POSTGIS_DEBUGF(1, "cb_updateNodes: %s", sql->data);

  spi_result = SPI_execute( sql->data, false, 0 );
  MemoryContextSwitchTo( oldcontext ); /* switch back */
  if ( spi_result != SPI_OK_UPDATE )
  {
    cberror(topo->be_data, "unexpected return (%d) from query execution: %s",
            spi_result, sql->data);
    pfree(sqldata.data);
    return -1;
  }
  pfree(sqldata.data);

  if ( SPI_processed ) topo->be_data->data_changed = true;

  POSTGIS_DEBUGF(1, "cb_updateNodes: update query processed " UINT64_FORMAT " rows", SPI_processed);

  return SPI_processed;
}

static int
cb_updateNodesById(const LWT_BE_TOPOLOGY *topo, const LWT_ISO_NODE *nodes, uint64_t numnodes, int fields)
{
  MemoryContext oldcontext = CurrentMemoryContext;
  uint64_t i;
  int spi_result;
  StringInfoData sqldata;
  StringInfo sql = &sqldata;
  const char *sep = "";
  const char *sep1 = ",";

  if ( ! fields )
  {
    cberror(topo->be_data,
            "updateNodesById callback called with no update fields!");
    return -1;
  }

  POSTGIS_DEBUGF(1,
		 "cb_updateNodesById got " UINT64_FORMAT
		 " nodes to update"
		 " (fields:%d)",
		 numnodes,
		 fields);

  initStringInfo(sql);
  appendStringInfoString(sql, "WITH newnodes(node_id,");
  addNodeFields(sql, fields);
  appendStringInfoString(sql, ") AS ( VALUES ");
  for (i=0; i<numnodes; ++i)
  {
    const LWT_ISO_NODE* node = &(nodes[i]);
    if ( i ) appendStringInfoString(sql, ",");
    addNodeValues(sql, node, LWT_COL_NODE_NODE_ID|fields);
  }
  appendStringInfo(sql, " ) UPDATE \"%s\".node n SET ", topo->name);

  /* TODO: turn the following into a function */
  if ( fields & LWT_COL_NODE_NODE_ID )
  {
    appendStringInfo(sql, "%snode_id = o.node_id", sep);
    sep = sep1;
  }
  if ( fields & LWT_COL_NODE_CONTAINING_FACE )
  {
    appendStringInfo(sql, "%scontaining_face = o.containing_face", sep);
    sep = sep1;
  }
  if ( fields & LWT_COL_NODE_GEOM )
  {
    appendStringInfo(sql, "%sgeom = o.geom", sep);
  }

  appendStringInfo(sql, " FROM newnodes o WHERE n.node_id = o.node_id");

  POSTGIS_DEBUGF(1, "cb_updateNodesById query: %s", sql->data);

  spi_result = SPI_execute( sql->data, false, 0 );
  MemoryContextSwitchTo( oldcontext ); /* switch back */
  if ( spi_result != SPI_OK_UPDATE )
  {
    cberror(topo->be_data, "unexpected return (%d) from query execution: %s",
            spi_result, sql->data);
    pfree(sqldata.data);
    return -1;
  }
  pfree(sqldata.data);

  if ( SPI_processed ) topo->be_data->data_changed = true;

  POSTGIS_DEBUGF(1, "cb_updateNodesById: update query processed " UINT64_FORMAT " rows", SPI_processed);

  return SPI_processed;
}

static uint64_t
cb_updateFacesById( const LWT_BE_TOPOLOGY* topo,
                    const LWT_ISO_FACE* faces, uint64_t numfaces )
{
  MemoryContext oldcontext = CurrentMemoryContext;
  uint64_t i;
  int spi_result;
  StringInfoData sqldata;
  StringInfo sql = &sqldata;

  initStringInfo(sql);
  appendStringInfoString(sql, "WITH newfaces(id,mbr) AS ( VALUES ");
  for (i=0; i<numfaces; ++i)
  {
    const LWT_ISO_FACE* face = &(faces[i]);
    char *hexbox = _box2d_to_hexwkb(face->mbr, topo->srid);

    if ( i ) appendStringInfoChar(sql, ',');

    appendStringInfo(sql, "(%" LWTFMT_ELEMID
                     ", ST_Envelope('%s'::geometry))",
                     face->face_id, hexbox);
    lwfree(hexbox);
  }
  appendStringInfo(sql, ") UPDATE \"%s\".face o SET mbr = i.mbr "
                   "FROM newfaces i WHERE o.face_id = i.id",
                   topo->name);

  POSTGIS_DEBUGF(1, "cb_updateFacesById query: %s", sql->data);

  spi_result = SPI_execute( sql->data, false, 0 );
  MemoryContextSwitchTo( oldcontext ); /* switch back */
  if ( spi_result != SPI_OK_UPDATE )
  {
    cberror(topo->be_data, "unexpected return (%d) from query execution: %s",
            spi_result, sql->data);
    pfree(sqldata.data);
    return UINT64_MAX;
  }
  pfree(sqldata.data);

  if ( SPI_processed ) topo->be_data->data_changed = true;

  POSTGIS_DEBUGF(1, "cb_updateFacesById: update query processed " UINT64_FORMAT " rows", SPI_processed);

  return SPI_processed;
}

static int
cb_updateEdgesById(const LWT_BE_TOPOLOGY *topo, const LWT_ISO_EDGE *edges, uint64_t numedges, int fields)
{
  MemoryContext oldcontext = CurrentMemoryContext;
  uint64_t i;
  int spi_result;
  StringInfoData sqldata;
  StringInfo sql = &sqldata;
  const char *sep = "";
  const char *sep1 = ",";

  if ( ! fields )
  {
    cberror(topo->be_data,
            "updateEdgesById callback called with no update fields!");
    return -1;
  }

  initStringInfo(sql);
  appendStringInfoString(sql, "WITH newedges(edge_id,");
  addEdgeFields(sql, fields, 0);
  appendStringInfoString(sql, ") AS ( VALUES ");
  for (i=0; i<numedges; ++i)
  {
    const LWT_ISO_EDGE* edge = &(edges[i]);
    if ( i ) appendStringInfoString(sql, ",");
    addEdgeValues(sql, edge, fields|LWT_COL_EDGE_EDGE_ID, 0);
  }
  appendStringInfo(sql, ") UPDATE \"%s\".edge_data e SET ", topo->name);

  /* TODO: turn the following into a function */
  if ( fields & LWT_COL_EDGE_START_NODE )
  {
    appendStringInfo(sql, "%sstart_node = o.start_node", sep);
    sep = sep1;
  }
  if ( fields & LWT_COL_EDGE_END_NODE )
  {
    appendStringInfo(sql, "%send_node = o.end_node", sep);
    sep = sep1;
  }
  if ( fields & LWT_COL_EDGE_FACE_LEFT )
  {
    appendStringInfo(sql, "%sleft_face = o.left_face", sep);
    sep = sep1;
  }
  if ( fields & LWT_COL_EDGE_FACE_RIGHT )
  {
    appendStringInfo(sql, "%sright_face = o.right_face", sep);
    sep = sep1;
  }
  if ( fields & LWT_COL_EDGE_NEXT_LEFT )
  {
    appendStringInfo(sql,
                     "%snext_left_edge = o.next_left_edge, "
                     "abs_next_left_edge = abs(o.next_left_edge)", sep);
    sep = sep1;
  }
  if ( fields & LWT_COL_EDGE_NEXT_RIGHT )
  {
    appendStringInfo(sql,
                     "%snext_right_edge = o.next_right_edge, "
                     "abs_next_right_edge = abs(o.next_right_edge)", sep);
    sep = sep1;
  }
  if ( fields & LWT_COL_EDGE_GEOM )
  {
    appendStringInfo(sql, "%sgeom = o.geom", sep);
  }

  appendStringInfo(sql, " FROM newedges o WHERE e.edge_id = o.edge_id");

  POSTGIS_DEBUGF(1, "cb_updateEdgesById query: %s", sql->data);

  spi_result = SPI_execute( sql->data, false, 0 );
  MemoryContextSwitchTo( oldcontext ); /* switch back */
  if ( spi_result != SPI_OK_UPDATE )
  {
    cberror(topo->be_data, "unexpected return (%d) from query execution: %s",
            spi_result, sql->data);
    pfree(sqldata.data);
    return -1;
  }
  pfree(sqldata.data);

  if ( SPI_processed ) topo->be_data->data_changed = true;

  POSTGIS_DEBUGF(1, "cb_updateEdgesById: update query processed " UINT64_FORMAT " rows", SPI_processed);

  return SPI_processed;
}

static int
cb_deleteEdges( const LWT_BE_TOPOLOGY* topo,
                const LWT_ISO_EDGE* sel_edge, int sel_fields )
{
  MemoryContext oldcontext = CurrentMemoryContext;
  int spi_result;
  StringInfoData sqldata;
  StringInfo sql = &sqldata;

  initStringInfo(sql);
  appendStringInfo(sql, "DELETE FROM \"%s\".edge_data WHERE ", topo->name);
  addEdgeUpdate( sql, sel_edge, sel_fields, 0, updSel );

  POSTGIS_DEBUGF(1, "cb_deleteEdges: %s", sql->data);

  spi_result = SPI_execute( sql->data, false, 0 );
  MemoryContextSwitchTo( oldcontext ); /* switch back */
  if ( spi_result != SPI_OK_DELETE )
  {
    cberror(topo->be_data, "unexpected return (%d) from query execution: %s",
            spi_result, sql->data);
    pfree(sqldata.data);
    return -1;
  }
  pfree(sqldata.data);

  if ( SPI_processed ) topo->be_data->data_changed = true;

  POSTGIS_DEBUGF(1, "cb_deleteEdges: delete query processed " UINT64_FORMAT " rows", SPI_processed);

  return SPI_processed;
}

static LWT_ELEMID
cb_getNextEdgeId( const LWT_BE_TOPOLOGY* topo )
{
  MemoryContext oldcontext = CurrentMemoryContext;
  int spi_result;
  StringInfoData sqldata;
  StringInfo sql = &sqldata;
  bool isnull;
  Datum dat;
  LWT_ELEMID edge_id;

  initStringInfo(sql);
  appendStringInfo(sql, "SELECT nextval('\"%s\".edge_data_edge_id_seq')",
                   topo->name);
  spi_result = SPI_execute(sql->data, false, 0);
  MemoryContextSwitchTo( oldcontext ); /* switch back */
  if ( spi_result != SPI_OK_SELECT )
  {
    cberror(topo->be_data, "unexpected return (%d) from query execution: %s",
            spi_result, sql->data);
    pfree(sqldata.data);
    return -1;
  }
  pfree(sqldata.data);

  if ( SPI_processed ) topo->be_data->data_changed = true;

  if ( SPI_processed != 1 )
  {
	  cberror(topo->be_data, "processed " UINT64_FORMAT " rows, expected 1", (uint64_t)SPI_processed);
	  return -1;
  }

  dat = SPI_getbinval( SPI_tuptable->vals[0],
                       SPI_tuptable->tupdesc, 1, &isnull );
  if ( isnull )
  {
    cberror(topo->be_data, "nextval for edge_id returned null");
    return -1;
  }
  edge_id = DatumGetInt64(dat); /* sequences return 64bit integers */

  SPI_freetuptable(SPI_tuptable);

  return edge_id;
}

static int
cb_updateTopoGeomEdgeSplit ( const LWT_BE_TOPOLOGY* topo,
                             LWT_ELEMID split_edge, LWT_ELEMID new_edge1, LWT_ELEMID new_edge2 )
{
  MemoryContext oldcontext = CurrentMemoryContext;
  int spi_result;
  StringInfoData sqldata;
  StringInfo sql = &sqldata;
  int i, ntopogeoms;
  const char *proj = "r.element_id, r.topogeo_id, r.layer_id, r.element_type";

  initStringInfo(sql);
  if ( new_edge2 == -1 )
  {
    appendStringInfo(sql, "SELECT %s", proj);
  }
  else
  {
    appendStringInfoString(sql, "DELETE");
  }
  appendStringInfo( sql, " FROM \"%s\".relation r %s topology.layer l WHERE "
                    "l.topology_id = %d AND l.level = 0 AND l.layer_id = r.layer_id "
                    "AND abs(r.element_id) = %" LWTFMT_ELEMID " AND r.element_type = 2",
                    topo->name, (new_edge2 == -1 ? "," : "USING" ), topo->id, split_edge );
  if ( new_edge2 != -1 )
  {
    appendStringInfo(sql, " RETURNING %s", proj);
  }

  POSTGIS_DEBUGF(1, "cb_updateTopoGeomEdgeSplit query: %s", sql->data);

  spi_result = SPI_execute(sql->data, new_edge2 == -1 ? !topo->be_data->data_changed : false, 0);
  MemoryContextSwitchTo( oldcontext ); /* switch back */
  if ( spi_result != ( new_edge2 == -1 ? SPI_OK_SELECT : SPI_OK_DELETE_RETURNING ) )
  {
    cberror(topo->be_data, "unexpected return (%d) from query execution: %s",
            spi_result, sql->data);
    pfree(sqldata.data);
    return 0;
  }

  if ( spi_result == SPI_OK_DELETE_RETURNING && SPI_processed )
  {
	  POSTGIS_DEBUGF(1, "cb_updateTopoGeomEdgeSplit: deleted " UINT64_FORMAT " faces", SPI_processed);
	  topo->be_data->data_changed = true;
  }

  ntopogeoms = SPI_processed;
  if ( ntopogeoms )
  {
    resetStringInfo(sql);
    appendStringInfo(sql, "INSERT INTO \"%s\".relation VALUES ", topo->name);
    for ( i=0; i<ntopogeoms; ++i )
    {
      HeapTuple row = SPI_tuptable->vals[i];
      TupleDesc tdesc = SPI_tuptable->tupdesc;
      int negate;
      int element_id;
      int topogeo_id;
      int layer_id;
      int element_type;

      if ( ! getNotNullInt32( row, tdesc, 1, &element_id ) )
      {
        cberror(topo->be_data,
                "unexpected null element_id in \"%s\".relation",
                topo->name);
        return 0;
      }
      negate = ( element_id < 0 );

      if ( ! getNotNullInt32( row, tdesc, 2, &topogeo_id ) )
      {
        cberror(topo->be_data,
                "unexpected null topogeo_id in \"%s\".relation",
                topo->name);
        return 0;
      }

      if ( ! getNotNullInt32( row, tdesc, 3, &layer_id ) )
      {
        cberror(topo->be_data,
                "unexpected null layer_id in \"%s\".relation",
                topo->name);
        return 0;
      }

      if ( ! getNotNullInt32( row, tdesc, 4, &element_type ) )
      {
        cberror(topo->be_data,
                "unexpected null element_type in \"%s\".relation",
                topo->name);
        return 0;
      }

      if ( i ) appendStringInfoChar(sql, ',');
      appendStringInfo(sql, "(%d,%d,%" LWTFMT_ELEMID ",%d)",
                       topogeo_id, layer_id, negate ? -new_edge1 : new_edge1, element_type);
      if ( new_edge2 != -1 )
      {
        resetStringInfo(sql);
        appendStringInfo(sql,
                         ",VALUES (%d,%d,%" LWTFMT_ELEMID ",%d",
                         topogeo_id, layer_id, negate ? -new_edge2 : new_edge2, element_type);
      }
    }

    SPI_freetuptable(SPI_tuptable);

    POSTGIS_DEBUGF(1, "cb_updateTopoGeomEdgeSplit query: %s", sql->data);
    spi_result = SPI_execute(sql->data, false, 0);
    MemoryContextSwitchTo( oldcontext ); /* switch back */
    if ( spi_result != SPI_OK_INSERT )
    {
      cberror(topo->be_data, "unexpected return (%d) from query execution: %s",
              spi_result, sql->data);
      pfree(sqldata.data);
      return 0;
    }
    if ( SPI_processed ) topo->be_data->data_changed = true;
  }

  POSTGIS_DEBUGF(1, "cb_updateTopoGeomEdgeSplit: updated %d topogeoms", ntopogeoms);

  pfree(sqldata.data);
  return 1;
}

static int
cb_updateTopoGeomFaceSplit ( const LWT_BE_TOPOLOGY* topo,
                             LWT_ELEMID split_face, LWT_ELEMID new_face1, LWT_ELEMID new_face2 )
{
  MemoryContext oldcontext = CurrentMemoryContext;
  int spi_result;
  StringInfoData sqldata;
  StringInfo sql = &sqldata;
  int i, ntopogeoms;
  const char *proj = "r.element_id, r.topogeo_id, r.layer_id, r.element_type";

  POSTGIS_DEBUGF(1, "cb_updateTopoGeomFaceSplit signalled "
                 "split of face %" LWTFMT_ELEMID " into %"
                 LWTFMT_ELEMID " and %" LWTFMT_ELEMID,
                 split_face, new_face1, new_face2);

  initStringInfo(sql);
  if ( new_face2 == -1 )
  {
    appendStringInfo(sql, "SELECT %s", proj);
  }
  else
  {
    appendStringInfoString(sql, "DELETE");
  }
  appendStringInfo( sql, " FROM \"%s\".relation r %s topology.layer l WHERE "
                    "l.topology_id = %d AND l.level = 0 AND l.layer_id = r.layer_id "
                    "AND abs(r.element_id) = %" LWTFMT_ELEMID " AND r.element_type = 3",
                    topo->name, (new_face2 == -1 ? "," : "USING" ), topo->id, split_face );
  if ( new_face2 != -1 )
  {
    appendStringInfo(sql, " RETURNING %s", proj);
  }

  POSTGIS_DEBUGF(1, "cb_updateTopoGeomFaceSplit query: %s", sql->data);

  spi_result = SPI_execute(sql->data, new_face2 == -1 ? !topo->be_data->data_changed : false, 0);
  MemoryContextSwitchTo( oldcontext ); /* switch back */
  if ( spi_result != ( new_face2 == -1 ? SPI_OK_SELECT : SPI_OK_DELETE_RETURNING ) )
  {
    cberror(topo->be_data, "unexpected return (%d) from query execution: %s",
            spi_result, sql->data);
    pfree(sqldata.data);
    return 0;
  }

  if ( spi_result == SPI_OK_DELETE_RETURNING && SPI_processed )
  {
    topo->be_data->data_changed = true;
  }

  ntopogeoms = SPI_processed;
  if ( ntopogeoms )
  {
    resetStringInfo(sql);
    appendStringInfo(sql, "INSERT INTO \"%s\".relation VALUES ", topo->name);
    for ( i=0; i<ntopogeoms; ++i )
    {
      HeapTuple row = SPI_tuptable->vals[i];
      TupleDesc tdesc = SPI_tuptable->tupdesc;
      int negate;
      int element_id;
      int topogeo_id;
      int layer_id;
      int element_type;

      if ( ! getNotNullInt32( row, tdesc, 1, &element_id ) )
      {
        cberror(topo->be_data,
                "unexpected null element_id in \"%s\".relation",
                topo->name);
        return 0;
      }
      negate = ( element_id < 0 );

      if ( ! getNotNullInt32( row, tdesc, 2, &topogeo_id ) )
      {
        cberror(topo->be_data,
                "unexpected null topogeo_id in \"%s\".relation",
                topo->name);
        return 0;
      }

      if ( ! getNotNullInt32( row, tdesc, 3, &layer_id ) )
      {
        cberror(topo->be_data,
                "unexpected null layer_id in \"%s\".relation",
                topo->name);
        return 0;
      }

      if ( ! getNotNullInt32( row, tdesc, 4, &element_type ) )
      {
        cberror(topo->be_data,
                "unexpected null element_type in \"%s\".relation",
                topo->name);
        return 0;
      }

      if ( i ) appendStringInfoChar(sql, ',');
      appendStringInfo(sql,
                       "(%d,%d,%" LWTFMT_ELEMID ",%d)",
                       topogeo_id, layer_id, negate ? -new_face1 : new_face1, element_type);

      if ( new_face2 != -1 )
      {
        appendStringInfo(sql,
                         ",(%d,%d,%" LWTFMT_ELEMID ",%d)",
                         topogeo_id, layer_id, negate ? -new_face2 : new_face2, element_type);
      }
    }

    SPI_freetuptable(SPI_tuptable);

    POSTGIS_DEBUGF(1, "cb_updateTopoGeomFaceSplit query: %s", sql->data);
    spi_result = SPI_execute(sql->data, false, 0);
    MemoryContextSwitchTo( oldcontext ); /* switch back */
    if ( spi_result != SPI_OK_INSERT )
    {
      cberror(topo->be_data, "unexpected return (%d) from query execution: %s",
              spi_result, sql->data);
      pfree(sqldata.data);
      return 0;
    }

    if ( SPI_processed ) topo->be_data->data_changed = true;
  }

  POSTGIS_DEBUGF(1, "cb_updateTopoGeomFaceSplit: updated %d topogeoms", ntopogeoms);

  pfree(sqldata.data);
  return 1;
}

static int
cb_checkTopoGeomRemEdge ( const LWT_BE_TOPOLOGY* topo,
                          LWT_ELEMID rem_edge, LWT_ELEMID face_left, LWT_ELEMID face_right )
{
  MemoryContext oldcontext = CurrentMemoryContext;
  int spi_result;
  StringInfoData sqldata;
  StringInfo sql = &sqldata;
  const char *tg_id, *layer_id;
  const char *schema_name, *table_name, *col_name;
  HeapTuple row;
  TupleDesc tdesc;

  POSTGIS_DEBUG(1, "cb_checkTopoGeomRemEdge enter ");

  initStringInfo(sql);
  appendStringInfo( sql, "SELECT r.topogeo_id, r.layer_id, "
                    "l.schema_name, l.table_name, l.feature_column FROM "
                    "topology.layer l INNER JOIN \"%s\".relation r "
                    "ON (l.layer_id = r.layer_id) WHERE l.level = 0 AND "
                    "l.feature_type = 2 AND l.topology_id = %d"
                    " AND abs(r.element_id) = %" LWTFMT_ELEMID,
                    topo->name, topo->id, rem_edge );

  POSTGIS_DEBUGF(1, "cb_checkTopoGeomRemEdge query 1: %s", sql->data);

  spi_result = SPI_execute(sql->data, !topo->be_data->data_changed, 0);
  MemoryContextSwitchTo( oldcontext ); /* switch back */
  if ( spi_result != SPI_OK_SELECT )
  {
    cberror(topo->be_data, "unexpected return (%d) from query execution: %s",
            spi_result, sql->data);
    pfree(sqldata.data);
    return 0;
  }

  if ( SPI_processed )
  {
    row = SPI_tuptable->vals[0];
    tdesc = SPI_tuptable->tupdesc;

    tg_id = SPI_getvalue(row, tdesc, 1);
    layer_id = SPI_getvalue(row, tdesc, 2);
    schema_name = SPI_getvalue(row, tdesc, 3);
    table_name = SPI_getvalue(row, tdesc, 4);
    col_name = SPI_getvalue(row, tdesc, 5);

    SPI_freetuptable(SPI_tuptable);

    cberror(topo->be_data, "TopoGeom %s in layer %s "
            "(%s.%s.%s) cannot be represented "
            "dropping edge %" LWTFMT_ELEMID,
            tg_id, layer_id, schema_name, table_name,
            col_name, rem_edge);
    return 0;
  }


  if ( face_left != face_right )
  {
    POSTGIS_DEBUGF(1, "Deletion of edge %" LWTFMT_ELEMID " joins faces %"
                   LWTFMT_ELEMID " and %" LWTFMT_ELEMID,
                   rem_edge, face_left, face_right);
    /*
      check if any topo_geom is defined only by one of the
      joined faces. In such case there would be no way to adapt
      the definition in case of healing, so we'd have to bail out
    */
    initStringInfo(sql);
    appendStringInfo( sql, "SELECT t.* FROM ( SELECT r.topogeo_id, "
                      "r.layer_id, l.schema_name, l.table_name, l.feature_column, "
                      "array_agg(r.element_id) as elems FROM topology.layer l "
                      " INNER JOIN \"%s\".relation r ON (l.layer_id = r.layer_id) "
                      "WHERE l.level = 0 and l.feature_type = 3 "
                      "AND l.topology_id = %d"
                      " AND r.element_id = ANY (ARRAY[%" LWTFMT_ELEMID ",%" LWTFMT_ELEMID
                      "]::int4[]) group by r.topogeo_id, r.layer_id, l.schema_name, "
                      "l.table_name, l.feature_column ) t WHERE NOT t.elems @> ARRAY[%"
                      LWTFMT_ELEMID ",%" LWTFMT_ELEMID "]::int4[]",

                      topo->name, topo->id,
                      face_left, face_right, face_left, face_right );

    POSTGIS_DEBUGF(1, "cb_checkTopoGeomRemEdge query 2: %s", sql->data);
    spi_result = SPI_execute(sql->data, !topo->be_data->data_changed, 0);
    MemoryContextSwitchTo( oldcontext ); /* switch back */

    if ( spi_result != SPI_OK_SELECT )
    {
      cberror(topo->be_data, "unexpected return (%d) from query execution: %s",
              spi_result, sql->data);
      pfree(sqldata.data);
      return 0;
    }

    if ( SPI_processed )
    {
      row = SPI_tuptable->vals[0];
      tdesc = SPI_tuptable->tupdesc;

      tg_id = SPI_getvalue(row, tdesc, 1);
      layer_id = SPI_getvalue(row, tdesc, 2);
      schema_name = SPI_getvalue(row, tdesc, 3);
      table_name = SPI_getvalue(row, tdesc, 4);
      col_name = SPI_getvalue(row, tdesc, 5);

      SPI_freetuptable(SPI_tuptable);

      cberror(topo->be_data, "TopoGeom %s in layer %s "
              "(%s.%s.%s) cannot be represented "
              "healing faces %" LWTFMT_ELEMID
              " and %" LWTFMT_ELEMID,
              tg_id, layer_id, schema_name, table_name,
              col_name, face_right, face_left);
      return 0;
    }
  }

  return 1;
}

static int
cb_checkTopoGeomRemNode ( const LWT_BE_TOPOLOGY* topo,
                          LWT_ELEMID rem_node, LWT_ELEMID edge1, LWT_ELEMID edge2 )
{
  MemoryContext oldcontext = CurrentMemoryContext;
  int spi_result;
  StringInfoData sqldata;
  StringInfo sql = &sqldata;
  const char *tg_id, *layer_id;
  const char *schema_name, *table_name, *col_name;
  HeapTuple row;
  TupleDesc tdesc;

  initStringInfo(sql);
  appendStringInfo( sql, "SELECT t.* FROM ( SELECT r.topogeo_id, "
                    "r.layer_id, l.schema_name, l.table_name, l.feature_column, "
                    "array_agg(abs(r.element_id)) as elems FROM topology.layer l "
                    " INNER JOIN \"%s\".relation r ON (l.layer_id = r.layer_id) "
                    "WHERE l.level = 0 and l.feature_type = 2 "
                    "AND l.topology_id = %d"
                    " AND abs(r.element_id) = ANY (ARRAY[%" LWTFMT_ELEMID ",%" LWTFMT_ELEMID
                    "]::int4[]) group by r.topogeo_id, r.layer_id, l.schema_name, "
                    "l.table_name, l.feature_column ) t WHERE NOT t.elems @> ARRAY[%"
                    LWTFMT_ELEMID ",%" LWTFMT_ELEMID "]::int4[]",
                    topo->name, topo->id,
                    edge1, edge2, edge1, edge2 );

  POSTGIS_DEBUGF(1, "cb_checkTopoGeomRemNode query 1: %s", sql->data);

  spi_result = SPI_execute(sql->data, !topo->be_data->data_changed, 0);
  MemoryContextSwitchTo( oldcontext ); /* switch back */
  if ( spi_result != SPI_OK_SELECT )
  {
    cberror(topo->be_data, "unexpected return (%d) from query execution: %s",
            spi_result, sql->data);
    pfree(sqldata.data);
    return 0;
  }

  if ( SPI_processed )
  {
    row = SPI_tuptable->vals[0];
    tdesc = SPI_tuptable->tupdesc;

    tg_id = SPI_getvalue(row, tdesc, 1);
    layer_id = SPI_getvalue(row, tdesc, 2);
    schema_name = SPI_getvalue(row, tdesc, 3);
    table_name = SPI_getvalue(row, tdesc, 4);
    col_name = SPI_getvalue(row, tdesc, 5);

    SPI_freetuptable(SPI_tuptable);

    cberror(topo->be_data, "TopoGeom %s in layer %s "
            "(%s.%s.%s) cannot be represented "
            "healing edges %" LWTFMT_ELEMID
            " and %" LWTFMT_ELEMID,
            tg_id, layer_id, schema_name, table_name,
            col_name, edge1, edge2);
    return 0;
  }

  /* TODO: check for TopoGeometry objects being defined by the common
   * node, see https://trac.osgeo.org/postgis/ticket/3239 */

  return 1;
}

static int
cb_updateTopoGeomFaceHeal ( const LWT_BE_TOPOLOGY* topo,
                            LWT_ELEMID face1, LWT_ELEMID face2, LWT_ELEMID newface )
{
  MemoryContext oldcontext = CurrentMemoryContext;
  int spi_result;
  StringInfoData sqldata;
  StringInfo sql = &sqldata;

  POSTGIS_DEBUG(1, "cb_updateTopoGeomFaceHeal enter ");

  /* delete oldfaces (not equal to newface) from the
   * set of primitives defining the TopoGeometries found before */

  if ( newface == face1 || newface == face2 )
  {
    initStringInfo(sql);
    /* this query can be optimized */
    appendStringInfo( sql, "DELETE FROM \"%s\".relation r "
                      "USING topology.layer l WHERE l.level = 0 AND l.feature_type = 3"
                      " AND l.topology_id = %d AND l.layer_id = r.layer_id "
                      " AND abs(r.element_id) IN ( %" LWTFMT_ELEMID ",%" LWTFMT_ELEMID ")"
                      " AND abs(r.element_id) != %" LWTFMT_ELEMID,
                      topo->name, topo->id, face1, face2, newface );
    POSTGIS_DEBUGF(1, "cb_updateTopoGeomFaceHeal query: %s", sql->data);

    spi_result = SPI_execute(sql->data, false, 0);
    MemoryContextSwitchTo( oldcontext ); /* switch back */
    if ( spi_result != SPI_OK_DELETE )
    {
      cberror(topo->be_data, "unexpected return (%d) from query execution: %s",
              spi_result, sql->data);
      pfree(sqldata.data);
      return 0;
    }
    if ( SPI_processed ) topo->be_data->data_changed = true;
  }
  else
  {
    initStringInfo(sql);
    /* delete face1 */
    appendStringInfo( sql, "DELETE FROM \"%s\".relation r "
                      "USING topology.layer l WHERE l.level = 0 AND l.feature_type = 3"
                      " AND l.topology_id = %d AND l.layer_id = r.layer_id "
                      " AND abs(r.element_id) = %" LWTFMT_ELEMID,
                      topo->name, topo->id, face1 );
    POSTGIS_DEBUGF(1, "cb_updateTopoGeomFaceHeal query 1: %s", sql->data);

    spi_result = SPI_execute(sql->data, false, 0);
    MemoryContextSwitchTo( oldcontext ); /* switch back */
    if ( spi_result != SPI_OK_DELETE )
    {
      cberror(topo->be_data, "unexpected return (%d) from query execution: %s",
              spi_result, sql->data);
      pfree(sqldata.data);
      return 0;
    }
    if ( SPI_processed ) topo->be_data->data_changed = true;

    initStringInfo(sql);
    /* update face2 to newface */
    appendStringInfo( sql, "UPDATE \"%s\".relation r "
                      "SET element_id = %" LWTFMT_ELEMID " FROM topology.layer l "
                      "WHERE l.level = 0 AND l.feature_type = 3 AND l.topology_id = %d"
                      " AND l.layer_id = r.layer_id AND r.element_id = %" LWTFMT_ELEMID,
                      topo->name, newface, topo->id, face2 );
    POSTGIS_DEBUGF(1, "cb_updateTopoGeomFaceHeal query 2: %s", sql->data);

    spi_result = SPI_execute(sql->data, false, 0);
    MemoryContextSwitchTo( oldcontext ); /* switch back */
    if ( spi_result != SPI_OK_UPDATE )
    {
      cberror(topo->be_data, "unexpected return (%d) from query execution: %s",
              spi_result, sql->data);
      pfree(sqldata.data);
      return 0;
    }
    if ( SPI_processed ) topo->be_data->data_changed = true;
  }

  return 1;
}

static int
cb_updateTopoGeomEdgeHeal ( const LWT_BE_TOPOLOGY* topo,
                            LWT_ELEMID edge1, LWT_ELEMID edge2, LWT_ELEMID newedge )
{
  MemoryContext oldcontext = CurrentMemoryContext;
  int spi_result;
  StringInfoData sqldata;
  StringInfo sql = &sqldata;

  /* delete old edges (not equal to new edge) from the
   * set of primitives defining the TopoGeometries found before */

  if ( newedge == edge1 || newedge == edge2 )
  {
    initStringInfo(sql);
    /* this query can be optimized */
    appendStringInfo( sql, "DELETE FROM \"%s\".relation r "
                      "USING topology.layer l WHERE l.level = 0 AND l.feature_type = 2"
                      " AND l.topology_id = %d AND l.layer_id = r.layer_id "
                      " AND abs(r.element_id) IN ( %" LWTFMT_ELEMID ",%" LWTFMT_ELEMID ")"
                      " AND abs(r.element_id) != %" LWTFMT_ELEMID,
                      topo->name, topo->id, edge1, edge2, newedge );
    POSTGIS_DEBUGF(1, "cb_updateTopoGeomEdgeHeal query: %s", sql->data);

    spi_result = SPI_execute(sql->data, false, 0);
    MemoryContextSwitchTo( oldcontext ); /* switch back */
    if ( spi_result != SPI_OK_DELETE )
    {
      cberror(topo->be_data, "unexpected return (%d) from query execution: %s",
              spi_result, sql->data);
      pfree(sqldata.data);
      return 0;
    }
    if ( SPI_processed ) topo->be_data->data_changed = true;
  }
  else
  {
    initStringInfo(sql);
    /* delete edge1 */
    appendStringInfo( sql, "DELETE FROM \"%s\".relation r "
                      "USING topology.layer l WHERE l.level = 0 AND l.feature_type = 2"
                      " AND l.topology_id = %d AND l.layer_id = r.layer_id "
                      " AND abs(r.element_id) = %" LWTFMT_ELEMID,
                      topo->name, topo->id, edge2 );
    POSTGIS_DEBUGF(1, "cb_updateTopoGeomEdgeHeal query 1: %s", sql->data);

    spi_result = SPI_execute(sql->data, false, 0);
    MemoryContextSwitchTo( oldcontext ); /* switch back */
    if ( spi_result != SPI_OK_DELETE )
    {
      cberror(topo->be_data, "unexpected return (%d) from query execution: %s",
              spi_result, sql->data);
      pfree(sqldata.data);
      return 0;
    }
    if ( SPI_processed ) topo->be_data->data_changed = true;

    initStringInfo(sql);
    /* update edge2 to newedge */
    appendStringInfo( sql, "UPDATE \"%s\".relation r "
                      "SET element_id = %" LWTFMT_ELEMID " *(element_id/%" LWTFMT_ELEMID
                      ") FROM topology.layer l "
                      "WHERE l.level = 0 AND l.feature_type = 2 AND l.topology_id = %d"
                      " AND l.layer_id = r.layer_id AND abs(r.element_id) = %" LWTFMT_ELEMID,
                      topo->name, newedge, edge1, topo->id, edge1 );
    POSTGIS_DEBUGF(1, "cb_updateTopoGeomEdgeHeal query 2: %s", sql->data);

    spi_result = SPI_execute(sql->data, false, 0);
    MemoryContextSwitchTo( oldcontext ); /* switch back */
    if ( spi_result != SPI_OK_UPDATE )
    {
      cberror(topo->be_data, "unexpected return (%d) from query execution: %s",
              spi_result, sql->data);
      pfree(sqldata.data);
      return 0;
    }
    if ( SPI_processed ) topo->be_data->data_changed = true;
  }

  return 1;
}

static LWT_ELEMID
cb_getFaceContainingPoint( const LWT_BE_TOPOLOGY* topo, const LWPOINT* pt )
{
  MemoryContext oldcontext = CurrentMemoryContext;
  int spi_result;
  StringInfoData sqldata;
  StringInfo sql = &sqldata;
  bool isnull;
  Datum dat;
  LWT_ELEMID face_id;
  GSERIALIZED *pts;
  Datum values[1];
  Oid argtypes[1];

  initStringInfo(sql);

  pts = geometry_serialize(lwpoint_as_lwgeom(pt));
  if ( ! pts )
  {
    cberror(topo->be_data, "%s:%d: could not serialize query point",
            __FILE__, __LINE__);
    return -2;
  }
  /* TODO: call GetFaceGeometry internally, avoiding the round-trip to sql */
  appendStringInfo(sql,
                   "WITH faces AS ( SELECT face_id FROM \"%s\".face "
                   "WHERE mbr && $1 ORDER BY ST_Area(mbr) ASC ) "
                   "SELECT face_id FROM faces WHERE _ST_Contains("
                   "topology.ST_GetFaceGeometry('%s', face_id), $1)"
                   " LIMIT 1",
                   topo->name, topo->name);

  values[0] = PointerGetDatum(pts);
  argtypes[0] = topo->geometryOID;
  spi_result = SPI_execute_with_args(sql->data, 1, argtypes, values, NULL,
                                     !topo->be_data->data_changed, 1);
  MemoryContextSwitchTo( oldcontext ); /* switch back */
  pfree(pts); /* not needed anymore */
  if ( spi_result != SPI_OK_SELECT )
  {
    cberror(topo->be_data, "unexpected return (%d) from query execution: %s",
            spi_result, sql->data);
    pfree(sqldata.data);
    return -2;
  }
  pfree(sqldata.data);

  if ( SPI_processed != 1 )
  {
    return -1; /* none found */
  }

  dat = SPI_getbinval( SPI_tuptable->vals[0],
                       SPI_tuptable->tupdesc, 1, &isnull );
  if ( isnull )
  {
    SPI_freetuptable(SPI_tuptable);
    cberror(topo->be_data, "corrupted topology: face with NULL face_id");
    return -2;
  }
  face_id = DatumGetInt32(dat);
  SPI_freetuptable(SPI_tuptable);
  return face_id;
}

static int
cb_deleteFacesById(const LWT_BE_TOPOLOGY *topo, const LWT_ELEMID *ids, uint64_t numelems)
{
  MemoryContext oldcontext = CurrentMemoryContext;
  int spi_result;
  StringInfoData sqldata;
  StringInfo sql = &sqldata;

  initStringInfo(sql);
  appendStringInfo(sql, "DELETE FROM \"%s\".face WHERE face_id IN (", topo->name);
  for (uint64_t i = 0; i < numelems; ++i)
  {
    appendStringInfo(sql, "%s%" LWTFMT_ELEMID, (i?",":""), ids[i]);
  }
  appendStringInfoString(sql, ")");

  POSTGIS_DEBUGF(1, "cb_deleteFacesById query: %s", sql->data);

  spi_result = SPI_execute( sql->data, false, 0 );
  MemoryContextSwitchTo( oldcontext ); /* switch back */
  if ( spi_result != SPI_OK_DELETE )
  {
    cberror(topo->be_data, "unexpected return (%d) from query execution: %s",
            spi_result, sql->data);
    pfree(sqldata.data);
    return -1;
  }
  pfree(sqldata.data);

  if ( SPI_processed ) topo->be_data->data_changed = true;

  POSTGIS_DEBUGF(1, "cb_deleteFacesById: delete query processed " UINT64_FORMAT " rows", SPI_processed);

  return SPI_processed;
}

static int
cb_deleteNodesById(const LWT_BE_TOPOLOGY *topo, const LWT_ELEMID *ids, uint64_t numelems)
{
  MemoryContext oldcontext = CurrentMemoryContext;
  int spi_result;
  StringInfoData sqldata;
  StringInfo sql = &sqldata;

  initStringInfo(sql);
  appendStringInfo(sql, "DELETE FROM \"%s\".node WHERE node_id IN (",
                   topo->name);
  for (uint64_t i = 0; i < numelems; ++i)
  {
    appendStringInfo(sql, "%s%" LWTFMT_ELEMID, (i?",":""), ids[i]);
  }
  appendStringInfoString(sql, ")");

  POSTGIS_DEBUGF(1, "cb_deleteNodesById query: %s", sql->data);

  spi_result = SPI_execute( sql->data, false, 0 );
  MemoryContextSwitchTo( oldcontext ); /* switch back */
  if ( spi_result != SPI_OK_DELETE )
  {
    cberror(topo->be_data, "unexpected return (%d) from query execution: %s",
            spi_result, sql->data);
    pfree(sqldata.data);
    return -1;
  }
  pfree(sqldata.data);

  if ( SPI_processed ) topo->be_data->data_changed = true;

  POSTGIS_DEBUGF(1, "cb_deleteNodesById: delete query processed " UINT64_FORMAT " rows", SPI_processed);

  return SPI_processed;
}

static LWT_ISO_NODE *
cb_getNodeWithinBox2D(const LWT_BE_TOPOLOGY *topo, const GBOX *box, uint64_t *numelems, int fields, int limit)
{
  MemoryContext oldcontext = CurrentMemoryContext;
  int spi_result;
  StringInfoData sqldata;
  StringInfo sql = &sqldata;
  uint64_t i;
  int elems_requested = limit;
  LWT_ISO_NODE* nodes;
  char *hexbox;

  initStringInfo(sql);

  if ( elems_requested == -1 )
  {
    appendStringInfoString(sql, "SELECT EXISTS ( SELECT 1");
  }
  else
  {
    appendStringInfoString(sql, "SELECT ");
    addNodeFields(sql, fields);
  }
  hexbox = _box2d_to_hexwkb(box, topo->srid);
  appendStringInfo(sql, " FROM \"%s\".node WHERE geom && '%s'::geometry",
                   topo->name, hexbox);
  lwfree(hexbox);
  if ( elems_requested == -1 )
  {
    appendStringInfoString(sql, ")");
  }
  else if ( elems_requested > 0 )
  {
    appendStringInfo(sql, " LIMIT %d", elems_requested);
  }
  POSTGIS_DEBUGF(1,"cb_getNodeWithinBox2D: query is: %s", sql->data);
  spi_result = SPI_execute(sql->data, !topo->be_data->data_changed, limit >= 0 ? limit : 0);
  MemoryContextSwitchTo( oldcontext ); /* switch back */
  if ( spi_result != SPI_OK_SELECT )
  {
    cberror(topo->be_data, "unexpected return (%d) from query execution: %s", spi_result, sql->data);
    pfree(sqldata.data);
    *numelems = UINT64_MAX;
    return NULL;
  }
  pfree(sqldata.data);

  POSTGIS_DEBUGF(1,
		 "cb_getNodeWithinBox2D: edge query "
		 "(limited by %d) returned " UINT64_FORMAT " rows",
		 elems_requested,
		 SPI_processed);
  *numelems = SPI_processed;
  if ( ! SPI_processed )
  {
    return NULL;
  }

  if ( elems_requested == -1 )
  {
    /* This was an EXISTS query */
    {
      Datum dat;
      bool isnull, exists;
      dat = SPI_getbinval(SPI_tuptable->vals[0], SPI_tuptable->tupdesc, 1, &isnull);
      exists = DatumGetBool(dat);
      SPI_freetuptable(SPI_tuptable);
      *numelems = exists ? 1 : 0;
      POSTGIS_DEBUGF(1, "cb_getNodeWithinBox2D: exists ? " UINT64_FORMAT, *numelems);
    }
    return NULL;
  }

  nodes = palloc( sizeof(LWT_ISO_EDGE) * *numelems );
  for ( i=0; i<*numelems; ++i )
  {
    HeapTuple row = SPI_tuptable->vals[i];
    fillNodeFields(&nodes[i], row, SPI_tuptable->tupdesc, fields);
  }

  SPI_freetuptable(SPI_tuptable);

  return nodes;
}

static LWT_ISO_EDGE *
cb_getEdgeWithinBox2D(const LWT_BE_TOPOLOGY *topo, const GBOX *box, uint64_t *numelems, int fields, int limit)
{
  MemoryContext oldcontext = CurrentMemoryContext;
  int spi_result;
  StringInfoData sqldata;
  StringInfo sql = &sqldata;
  uint64_t i;
  int elems_requested = limit;
  LWT_ISO_EDGE* edges;
  char *hexbox;

  initStringInfo(sql);

  if ( elems_requested == -1 )
  {
    appendStringInfoString(sql, "SELECT EXISTS ( SELECT 1");
  }
  else
  {
    appendStringInfoString(sql, "SELECT ");
    addEdgeFields(sql, fields, 0);
  }
  appendStringInfo(sql, " FROM \"%s\".edge", topo->name);

  if ( box )
  {
    hexbox = _box2d_to_hexwkb(box, topo->srid);
    appendStringInfo(sql, " WHERE geom && '%s'::geometry", hexbox);
    lwfree(hexbox);
  }

  if ( elems_requested == -1 )
  {
    appendStringInfoString(sql, ")");
  }
  else if ( elems_requested > 0 )
  {
    appendStringInfo(sql, " LIMIT %d", elems_requested);
  }
  POSTGIS_DEBUGF(1,"cb_getEdgeWithinBox2D: query is: %s", sql->data);
  spi_result = SPI_execute(sql->data, !topo->be_data->data_changed, limit >= 0 ? limit : 0);
  MemoryContextSwitchTo( oldcontext ); /* switch back */
  if ( spi_result != SPI_OK_SELECT )
  {
    cberror(topo->be_data, "unexpected return (%d) from query execution: %s", spi_result, sql->data);
    pfree(sqldata.data);
    *numelems = UINT64_MAX;
    return NULL;
  }
  pfree(sqldata.data);

  POSTGIS_DEBUGF(1,
		 "cb_getEdgeWithinBox2D: edge query "
		 "(limited by %d) returned " UINT64_FORMAT " rows",
		 elems_requested,
		 SPI_processed);
  *numelems = SPI_processed;
  if ( ! SPI_processed )
  {
    return NULL;
  }

  if ( elems_requested == -1 )
  {
    /* This was an EXISTS query */
    {
      Datum dat;
      bool isnull, exists;
      dat = SPI_getbinval(SPI_tuptable->vals[0], SPI_tuptable->tupdesc, 1, &isnull);
      exists = DatumGetBool(dat);
      *numelems = exists ? 1 : 0;
      SPI_freetuptable(SPI_tuptable);
      POSTGIS_DEBUGF(1, "cb_getEdgeWithinBox2D: exists ? " UINT64_FORMAT, *numelems);
    }
    return NULL;
  }

  edges = palloc( sizeof(LWT_ISO_EDGE) * *numelems );
  for ( i=0; i<*numelems; ++i )
  {
    HeapTuple row = SPI_tuptable->vals[i];
    fillEdgeFields(&edges[i], row, SPI_tuptable->tupdesc, fields);
  }

  SPI_freetuptable(SPI_tuptable);

  return edges;
}

static LWT_ISO_FACE *
cb_getFaceWithinBox2D(const LWT_BE_TOPOLOGY *topo, const GBOX *box, uint64_t *numelems, int fields, int limit)
{
  MemoryContext oldcontext = CurrentMemoryContext;
  int spi_result;
  StringInfoData sqldata;
  StringInfo sql = &sqldata;
  uint64_t i;
  int elems_requested = limit;
  LWT_ISO_FACE* faces;
  char *hexbox;

  initStringInfo(sql);

  if ( elems_requested == -1 )
  {
    appendStringInfoString(sql, "SELECT EXISTS ( SELECT 1");
  }
  else
  {
    appendStringInfoString(sql, "SELECT ");
    addFaceFields(sql, fields);
  }
  hexbox = _box2d_to_hexwkb(box, topo->srid);
  appendStringInfo(sql, " FROM \"%s\".face WHERE mbr && '%s'::geometry",
                   topo->name, hexbox);
  lwfree(hexbox);
  if ( elems_requested == -1 )
  {
    appendStringInfoString(sql, ")");
  }
  else if ( elems_requested > 0 )
  {
    appendStringInfo(sql, " LIMIT %d", elems_requested);
  }
  POSTGIS_DEBUGF(1,"cb_getFaceWithinBox2D: query is: %s", sql->data);
  spi_result = SPI_execute(sql->data, !topo->be_data->data_changed, limit >= 0 ? limit : 0);
  MemoryContextSwitchTo( oldcontext ); /* switch back */
  if ( spi_result != SPI_OK_SELECT )
  {
    cberror(topo->be_data, "unexpected return (%d) from query execution: %s", spi_result, sql->data);
    pfree(sqldata.data);
    *numelems = UINT64_MAX;
    return NULL;
  }
  pfree(sqldata.data);

  POSTGIS_DEBUGF(1,
		 "cb_getFaceWithinBox2D: face query "
		 "(limited by %d) returned " UINT64_FORMAT " rows",
		 elems_requested,
		 SPI_processed);
  *numelems = SPI_processed;
  if ( ! SPI_processed )
  {
    return NULL;
  }

  if ( elems_requested == -1 )
  {
    /* This was an EXISTS query */
    {
      Datum dat;
      bool isnull, exists;
      dat = SPI_getbinval(SPI_tuptable->vals[0], SPI_tuptable->tupdesc, 1, &isnull);
      exists = DatumGetBool(dat);
      *numelems = exists ? 1 : 0;
      POSTGIS_DEBUGF(1, "cb_getFaceWithinBox2D: exists ? " UINT64_FORMAT, *numelems);
    }

    SPI_freetuptable(SPI_tuptable);

    return NULL;
  }

  faces = palloc( sizeof(LWT_ISO_EDGE) * *numelems );
  for ( i=0; i<*numelems; ++i )
  {
    HeapTuple row = SPI_tuptable->vals[i];
    fillFaceFields(&faces[i], row, SPI_tuptable->tupdesc, fields);
  }

  SPI_freetuptable(SPI_tuptable);

  return faces;
}


static LWT_BE_CALLBACKS be_callbacks =
{
  cb_lastErrorMessage,
  NULL, /* createTopology */
  cb_loadTopologyByName,
  cb_freeTopology,
  cb_getNodeById,
  cb_getNodeWithinDistance2D,
  cb_insertNodes,
  cb_getEdgeById,
  cb_getEdgeWithinDistance2D,
  cb_getNextEdgeId,
  cb_insertEdges,
  cb_updateEdges,
  cb_getFacesById,
  cb_getFaceContainingPoint,
  cb_updateTopoGeomEdgeSplit,
  cb_deleteEdges,
  cb_getNodeWithinBox2D,
  cb_getEdgeWithinBox2D,
  cb_getEdgeByNode,
  cb_updateNodes,
  cb_updateTopoGeomFaceSplit,
  cb_insertFaces,
  cb_updateFacesById,
  cb_getRingEdges,
  cb_updateEdgesById,
  cb_getEdgeByFace,
  cb_getNodeByFace,
  cb_updateNodesById,
  cb_deleteFacesById,
  cb_topoGetSRID,
  cb_topoGetPrecision,
  cb_topoHasZ,
  cb_deleteNodesById,
  cb_checkTopoGeomRemEdge,
  cb_updateTopoGeomFaceHeal,
  cb_checkTopoGeomRemNode,
  cb_updateTopoGeomEdgeHeal,
  cb_getFaceWithinBox2D
};

static void
xact_callback(XactEvent event, void *arg)
{
  LWT_BE_DATA* data = (LWT_BE_DATA *)arg;
  POSTGIS_DEBUGF(1, "xact_callback called with event %d", event);
  data->data_changed = false;
}


/*
 * Module load callback
 */
void _PG_init(void);
void
_PG_init(void)
{
  MemoryContext old_context;

  /*
   * install PostgreSQL handlers for liblwgeom
   * NOTE: they may be already in place!
   */
  pg_install_lwgeom_handlers();

  /* Switch to the top memory context so that the backend interface
   * is valid for the whole backend lifetime */
  old_context = MemoryContextSwitchTo( TopMemoryContext );

  /* initialize backend data */
  be_data.data_changed = false;
  be_data.topoLoadFailMessageFlavor = 0;

  /* hook on transaction end to reset data_changed */
  RegisterXactCallback(xact_callback, &be_data);

  /* register callbacks against liblwgeom-topo */
  be_iface = lwt_CreateBackendIface(&be_data);
  lwt_BackendIfaceRegisterCallbacks(be_iface, &be_callbacks);

  /* Switch back to whatever memory context was in place
   * at time of _PG_init enter.
   * See http://www.postgresql.org/message-id/20150623114125.GD5835@localhost
   */
  MemoryContextSwitchTo(old_context);
}

/*
 * Module unload callback
 */
void _PG_fini(void);
void
_PG_fini(void)
{
  elog(NOTICE, "Goodbye from PostGIS Topology %s", POSTGIS_VERSION);

  UnregisterXactCallback(xact_callback, &be_data);
  lwt_FreeBackendIface(be_iface);
}

/*  ST_ModEdgeSplit(atopology, anedge, apoint) */
Datum ST_ModEdgeSplit(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(ST_ModEdgeSplit);
Datum ST_ModEdgeSplit(PG_FUNCTION_ARGS)
{
  text* toponame_text;
  char* toponame;
  LWT_ELEMID edge_id;
  LWT_ELEMID node_id;
  GSERIALIZED *geom;
  LWGEOM *lwgeom;
  LWPOINT *pt;
  LWT_TOPOLOGY *topo;

  if ( PG_ARGISNULL(0) || PG_ARGISNULL(1) || PG_ARGISNULL(2) )
  {
    lwpgerror("SQL/MM Spatial exception - null argument");
    PG_RETURN_NULL();
  }

  toponame_text = PG_GETARG_TEXT_P(0);
  toponame = text_to_cstring(toponame_text);
  PG_FREE_IF_COPY(toponame_text, 0);

  edge_id = PG_GETARG_INT32(1) ;

  geom = PG_GETARG_GSERIALIZED_P(2);
  lwgeom = lwgeom_from_gserialized(geom);
  pt = lwgeom_as_lwpoint(lwgeom);
  if ( ! pt )
  {
    lwgeom_free(lwgeom);
    PG_FREE_IF_COPY(geom, 2);
    lwpgerror("ST_ModEdgeSplit third argument must be a point geometry");
    PG_RETURN_NULL();
  }

  if ( SPI_OK_CONNECT != SPI_connect() )
  {
    lwpgerror("Could not connect to SPI");
    PG_RETURN_NULL();
  }

  topo = lwt_LoadTopology(be_iface, toponame);
  pfree(toponame);
  if ( ! topo )
  {
    /* should never reach this point, as lwerror would raise an exception */
    SPI_finish();
    PG_RETURN_NULL();
  }

  POSTGIS_DEBUG(1, "Calling lwt_ModEdgeSplit");
  node_id = lwt_ModEdgeSplit(topo, edge_id, pt, 0);
  POSTGIS_DEBUG(1, "lwt_ModEdgeSplit returned");
  lwgeom_free(lwgeom);
  PG_FREE_IF_COPY(geom, 3);
  lwt_FreeTopology(topo);

  if ( node_id == -1 )
  {
    /* should never reach this point, as lwerror would raise an exception */
    SPI_finish();
    PG_RETURN_NULL();
  }

  SPI_finish();
  PG_RETURN_INT32(node_id);
}

/*  ST_NewEdgesSplit(atopology, anedge, apoint) */
Datum ST_NewEdgesSplit(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(ST_NewEdgesSplit);
Datum ST_NewEdgesSplit(PG_FUNCTION_ARGS)
{
  text* toponame_text;
  char* toponame;
  LWT_ELEMID edge_id;
  LWT_ELEMID node_id;
  GSERIALIZED *geom;
  LWGEOM *lwgeom;
  LWPOINT *pt;
  LWT_TOPOLOGY *topo;

  if ( PG_ARGISNULL(0) || PG_ARGISNULL(1) || PG_ARGISNULL(2) )
  {
    lwpgerror("SQL/MM Spatial exception - null argument");
    PG_RETURN_NULL();
  }

  toponame_text = PG_GETARG_TEXT_P(0);
  toponame = text_to_cstring(toponame_text);
  PG_FREE_IF_COPY(toponame_text, 0);

  edge_id = PG_GETARG_INT32(1) ;

  geom = PG_GETARG_GSERIALIZED_P(2);
  lwgeom = lwgeom_from_gserialized(geom);
  pt = lwgeom_as_lwpoint(lwgeom);
  if ( ! pt )
  {
    lwgeom_free(lwgeom);
    PG_FREE_IF_COPY(geom, 2);
    lwpgerror("ST_NewEdgesSplit third argument must be a point geometry");
    PG_RETURN_NULL();
  }

  if ( SPI_OK_CONNECT != SPI_connect() )
  {
    lwpgerror("Could not connect to SPI");
    PG_RETURN_NULL();
  }

  topo = lwt_LoadTopology(be_iface, toponame);
  pfree(toponame);
  if ( ! topo )
  {
    /* should never reach this point, as lwerror would raise an exception */
    SPI_finish();
    PG_RETURN_NULL();
  }

  POSTGIS_DEBUG(1, "Calling lwt_NewEdgesSplit");
  node_id = lwt_NewEdgesSplit(topo, edge_id, pt, 0);
  POSTGIS_DEBUG(1, "lwt_NewEdgesSplit returned");
  lwgeom_free(lwgeom);
  PG_FREE_IF_COPY(geom, 3);
  lwt_FreeTopology(topo);

  if ( node_id == -1 )
  {
    /* should never reach this point, as lwerror would raise an exception */
    SPI_finish();
    PG_RETURN_NULL();
  }

  SPI_finish();
  PG_RETURN_INT32(node_id);
}

/*  ST_AddIsoNode(atopology, aface, apoint) */
Datum ST_AddIsoNode(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(ST_AddIsoNode);
Datum ST_AddIsoNode(PG_FUNCTION_ARGS)
{
  text* toponame_text;
  char* toponame;
  LWT_ELEMID containing_face;
  LWT_ELEMID node_id;
  GSERIALIZED *geom;
  LWGEOM *lwgeom;
  LWPOINT *pt;
  LWT_TOPOLOGY *topo;

  if ( PG_ARGISNULL(0) || PG_ARGISNULL(2) )
  {
    lwpgerror("SQL/MM Spatial exception - null argument");
    PG_RETURN_NULL();
  }

  toponame_text = PG_GETARG_TEXT_P(0);
  toponame = text_to_cstring(toponame_text);
  PG_FREE_IF_COPY(toponame_text, 0);

  if ( PG_ARGISNULL(1) ) containing_face = -1;
  else
  {
    containing_face = PG_GETARG_INT32(1);
    if ( containing_face < 0 )
    {
      lwpgerror("SQL/MM Spatial exception - not within face");
      PG_RETURN_NULL();
    }
  }

  geom = PG_GETARG_GSERIALIZED_P(2);
  lwgeom = lwgeom_from_gserialized(geom);
  pt = lwgeom_as_lwpoint(lwgeom);
  if ( ! pt )
  {
    lwgeom_free(lwgeom);
    PG_FREE_IF_COPY(geom, 2);
    lwpgerror("SQL/MM Spatial exception - invalid point");
    PG_RETURN_NULL();
  }
  if ( lwpoint_is_empty(pt) )
  {
    lwgeom_free(lwgeom);
    PG_FREE_IF_COPY(geom, 2);
    lwpgerror("SQL/MM Spatial exception - empty point");
    PG_RETURN_NULL();
  }

  if ( SPI_OK_CONNECT != SPI_connect() )
  {
    lwpgerror("Could not connect to SPI");
    PG_RETURN_NULL();
  }

  topo = lwt_LoadTopology(be_iface, toponame);
  pfree(toponame);
  if ( ! topo )
  {
    /* should never reach this point, as lwerror would raise an exception */
    SPI_finish();
    PG_RETURN_NULL();
  }

  POSTGIS_DEBUG(1, "Calling lwt_AddIsoNode");
  node_id = lwt_AddIsoNode(topo, containing_face, pt, 0);
  POSTGIS_DEBUG(1, "lwt_AddIsoNode returned");
  lwgeom_free(lwgeom);
  PG_FREE_IF_COPY(geom, 2);
  lwt_FreeTopology(topo);

  if ( node_id == -1 )
  {
    /* should never reach this point, as lwerror would raise an exception */
    SPI_finish();
    PG_RETURN_NULL();
  }

  SPI_finish();
  PG_RETURN_INT32(node_id);
}

/*  ST_AddIsoEdge(atopology, anode, anothernode, acurve) */
Datum ST_AddIsoEdge(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(ST_AddIsoEdge);
Datum ST_AddIsoEdge(PG_FUNCTION_ARGS)
{
  text* toponame_text;
  char* toponame;
  LWT_ELEMID edge_id;
  LWT_ELEMID start_node, end_node;
  GSERIALIZED *geom;
  LWGEOM *lwgeom;
  LWLINE *curve;
  LWT_TOPOLOGY *topo;

  if ( PG_ARGISNULL(0) || PG_ARGISNULL(1) ||
       PG_ARGISNULL(2) || PG_ARGISNULL(3) )
  {
    lwpgerror("SQL/MM Spatial exception - null argument");
    PG_RETURN_NULL();
  }

  toponame_text = PG_GETARG_TEXT_P(0);
  toponame = text_to_cstring(toponame_text);
  PG_FREE_IF_COPY(toponame_text, 0);

  start_node = PG_GETARG_INT32(1);
  end_node = PG_GETARG_INT32(2);

  if ( start_node == end_node )
  {
    lwpgerror("Closed edges would not be isolated, try ST_AddEdgeNewFaces");
    PG_RETURN_NULL();
  }

  geom = PG_GETARG_GSERIALIZED_P(3);
  lwgeom = lwgeom_from_gserialized(geom);
  curve = lwgeom_as_lwline(lwgeom);
  if ( ! curve )
  {
    lwgeom_free(lwgeom);
    PG_FREE_IF_COPY(geom, 3);
    lwpgerror("SQL/MM Spatial exception - invalid curve");
    PG_RETURN_NULL();
  }

  if ( SPI_OK_CONNECT != SPI_connect() )
  {
    lwpgerror("Could not connect to SPI");
    PG_RETURN_NULL();
  }

  topo = lwt_LoadTopology(be_iface, toponame);
  pfree(toponame);
  if ( ! topo )
  {
    /* should never reach this point, as lwerror would raise an exception */
    SPI_finish();
    PG_RETURN_NULL();
  }

  POSTGIS_DEBUG(1, "Calling lwt_AddIsoEdge");
  edge_id = lwt_AddIsoEdge(topo, start_node, end_node, curve);
  POSTGIS_DEBUG(1, "lwt_AddIsoNode returned");
  lwgeom_free(lwgeom);
  PG_FREE_IF_COPY(geom, 3);
  lwt_FreeTopology(topo);

  if ( edge_id == -1 )
  {
    /* should never reach this point, as lwerror would raise an exception */
    SPI_finish();
    PG_RETURN_NULL();
  }

  SPI_finish();
  PG_RETURN_INT32(edge_id);
}

/*  ST_AddEdgeModFace(atopology, snode, enode, line) */
Datum ST_AddEdgeModFace(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(ST_AddEdgeModFace);
Datum ST_AddEdgeModFace(PG_FUNCTION_ARGS)
{
  text* toponame_text;
  char* toponame;
  LWT_ELEMID startnode_id, endnode_id;
  int edge_id;
  GSERIALIZED *geom;
  LWGEOM *lwgeom;
  LWLINE *line;
  LWT_TOPOLOGY *topo;

  if ( PG_ARGISNULL(0) || PG_ARGISNULL(1) || PG_ARGISNULL(2) || PG_ARGISNULL(3) )
  {
    lwpgerror("SQL/MM Spatial exception - null argument");
    PG_RETURN_NULL();
  }

  toponame_text = PG_GETARG_TEXT_P(0);
  toponame = text_to_cstring(toponame_text);
  PG_FREE_IF_COPY(toponame_text, 0);

  startnode_id = PG_GETARG_INT32(1) ;
  endnode_id = PG_GETARG_INT32(2) ;

  geom = PG_GETARG_GSERIALIZED_P(3);
  lwgeom = lwgeom_from_gserialized(geom);
  line = lwgeom_as_lwline(lwgeom);
  if ( ! line )
  {
    lwgeom_free(lwgeom);
    PG_FREE_IF_COPY(geom, 3);
    lwpgerror("ST_AddEdgeModFace fourth argument must be a line geometry");
    PG_RETURN_NULL();
  }

  if ( SPI_OK_CONNECT != SPI_connect() )
  {
    lwpgerror("Could not connect to SPI");
    PG_RETURN_NULL();
  }

  topo = lwt_LoadTopology(be_iface, toponame);
  pfree(toponame);
  if ( ! topo )
  {
    /* should never reach this point, as lwerror would raise an exception */
    SPI_finish();
    PG_RETURN_NULL();
  }

  POSTGIS_DEBUG(1, "Calling lwt_AddEdgeModFace");
  edge_id = lwt_AddEdgeModFace(topo, startnode_id, endnode_id, line, 0);
  POSTGIS_DEBUG(1, "lwt_AddEdgeModFace returned");
  lwgeom_free(lwgeom);
  PG_FREE_IF_COPY(geom, 3);
  lwt_FreeTopology(topo);

  if ( edge_id == -1 )
  {
    /* should never reach this point, as lwerror would raise an exception */
    SPI_finish();
    PG_RETURN_NULL();
  }

  SPI_finish();
  PG_RETURN_INT32(edge_id);
}

/*  ST_AddEdgeNewFaces(atopology, snode, enode, line) */
Datum ST_AddEdgeNewFaces(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(ST_AddEdgeNewFaces);
Datum ST_AddEdgeNewFaces(PG_FUNCTION_ARGS)
{
  text* toponame_text;
  char* toponame;
  LWT_ELEMID startnode_id, endnode_id;
  int edge_id;
  GSERIALIZED *geom;
  LWGEOM *lwgeom;
  LWLINE *line;
  LWT_TOPOLOGY *topo;

  if ( PG_ARGISNULL(0) || PG_ARGISNULL(1) || PG_ARGISNULL(2) || PG_ARGISNULL(3) )
  {
    lwpgerror("SQL/MM Spatial exception - null argument");
    PG_RETURN_NULL();
  }

  toponame_text = PG_GETARG_TEXT_P(0);
  toponame = text_to_cstring(toponame_text);
  PG_FREE_IF_COPY(toponame_text, 0);

  startnode_id = PG_GETARG_INT32(1) ;
  endnode_id = PG_GETARG_INT32(2) ;

  geom = PG_GETARG_GSERIALIZED_P(3);
  lwgeom = lwgeom_from_gserialized(geom);
  line = lwgeom_as_lwline(lwgeom);
  if ( ! line )
  {
    lwgeom_free(lwgeom);
    PG_FREE_IF_COPY(geom, 3);
    lwpgerror("ST_AddEdgeModFace fourth argument must be a line geometry");
    PG_RETURN_NULL();
  }

  if ( SPI_OK_CONNECT != SPI_connect() )
  {
    lwpgerror("Could not connect to SPI");
    PG_RETURN_NULL();
  }

  topo = lwt_LoadTopology(be_iface, toponame);
  pfree(toponame);
  if ( ! topo )
  {
    /* should never reach this point, as lwerror would raise an exception */
    SPI_finish();
    PG_RETURN_NULL();
  }

  POSTGIS_DEBUG(1, "Calling lwt_AddEdgeNewFaces");
  edge_id = lwt_AddEdgeNewFaces(topo, startnode_id, endnode_id, line, 0);
  POSTGIS_DEBUG(1, "lwt_AddEdgeNewFaces returned");
  lwgeom_free(lwgeom);
  PG_FREE_IF_COPY(geom, 3);
  lwt_FreeTopology(topo);

  if ( edge_id == -1 )
  {
    /* should never reach this point, as lwerror would raise an exception */
    SPI_finish();
    PG_RETURN_NULL();
  }

  SPI_finish();
  PG_RETURN_INT32(edge_id);
}

/* ST_GetFaceGeometry(atopology, aface) */
Datum ST_GetFaceGeometry(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(ST_GetFaceGeometry);
Datum ST_GetFaceGeometry(PG_FUNCTION_ARGS)
{
  text* toponame_text;
  char* toponame;
  LWT_ELEMID face_id;
  LWGEOM *lwgeom;
  LWT_TOPOLOGY *topo;
  GSERIALIZED *geom;
  MemoryContext old_context;

  if ( PG_ARGISNULL(0) || PG_ARGISNULL(1) )
  {
    lwpgerror("SQL/MM Spatial exception - null argument");
    PG_RETURN_NULL();
  }

  toponame_text = PG_GETARG_TEXT_P(0);
  toponame = text_to_cstring(toponame_text);
  PG_FREE_IF_COPY(toponame_text, 0);

  face_id = PG_GETARG_INT32(1) ;

  if ( SPI_OK_CONNECT != SPI_connect() )
  {
    lwpgerror("Could not connect to SPI");
    PG_RETURN_NULL();
  }

  topo = lwt_LoadTopology(be_iface, toponame);
  pfree(toponame);
  if ( ! topo )
  {
    /* should never reach this point, as lwerror would raise an exception */
    SPI_finish();
    PG_RETURN_NULL();
  }

  POSTGIS_DEBUG(1, "Calling lwt_GetFaceGeometry");
  lwgeom = lwt_GetFaceGeometry(topo, face_id);
  POSTGIS_DEBUG(1, "lwt_GetFaceGeometry returned");
  lwt_FreeTopology(topo);

  if ( lwgeom == NULL )
  {
    /* should never reach this point, as lwerror would raise an exception */
    SPI_finish();
    PG_RETURN_NULL();
  }

  /* Serialize in upper memory context (outside of SPI) */
  /* TODO: use a narrower context to switch to */
  old_context = MemoryContextSwitchTo( TopMemoryContext );
  geom = geometry_serialize(lwgeom);
  MemoryContextSwitchTo(old_context);

  SPI_finish();

  PG_RETURN_POINTER(geom);
}

typedef struct FACEEDGESSTATE
{
  LWT_ELEMID *elems;
  int nelems;
  int curr;
}
FACEEDGESSTATE;

/* ST_GetFaceEdges(atopology, aface) */
Datum ST_GetFaceEdges(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(ST_GetFaceEdges);
Datum ST_GetFaceEdges(PG_FUNCTION_ARGS)
{
  text* toponame_text;
  char* toponame;
  LWT_ELEMID face_id;
  int nelems;
  LWT_ELEMID *elems;
  LWT_TOPOLOGY *topo;
  FuncCallContext *funcctx;
  MemoryContext oldcontext, newcontext;
  TupleDesc tupdesc;
  HeapTuple tuple;
  AttInMetadata *attinmeta;
  FACEEDGESSTATE *state;
  char buf[64];
  char *values[2];
  Datum result;

  values[0] = buf;
  values[1] = &(buf[32]);

  if (SRF_IS_FIRSTCALL())
  {

    POSTGIS_DEBUG(1, "ST_GetFaceEdges first call");
    funcctx = SRF_FIRSTCALL_INIT();
    newcontext = funcctx->multi_call_memory_ctx;

    if ( PG_ARGISNULL(0) || PG_ARGISNULL(1) )
    {
      lwpgerror("SQL/MM Spatial exception - null argument");
      PG_RETURN_NULL();
    }

    toponame_text = PG_GETARG_TEXT_P(0);
    toponame = text_to_cstring(toponame_text);
    PG_FREE_IF_COPY(toponame_text, 0);

    face_id = PG_GETARG_INT32(1) ;

    if ( SPI_OK_CONNECT != SPI_connect() )
    {
      lwpgerror("Could not connect to SPI");
      PG_RETURN_NULL();
    }

    topo = lwt_LoadTopology(be_iface, toponame);
    oldcontext = MemoryContextSwitchTo( newcontext );
    pfree(toponame);
    if ( ! topo )
    {
      /* should never reach this point, as lwerror would raise an exception */
      SPI_finish();
      PG_RETURN_NULL();
    }

    POSTGIS_DEBUG(1, "Calling lwt_GetFaceEdges");
    nelems = lwt_GetFaceEdges(topo, face_id, &elems);
    POSTGIS_DEBUGF(1, "lwt_GetFaceEdges returned %d", nelems);
    lwt_FreeTopology(topo);

    if ( nelems < 0 )
    {
      /* should never reach this point, as lwerror would raise an exception */
      SPI_finish();
      PG_RETURN_NULL();
    }

    state = lwalloc(sizeof(FACEEDGESSTATE));
    state->elems = elems;
    state->nelems = nelems;
    state->curr = 0;
    funcctx->user_fctx = state;

    /*
     * Build a tuple description for a
     * getfaceedges_returntype tuple
     */
    tupdesc = RelationNameGetTupleDesc("topology.getfaceedges_returntype");

    /*
     * generate attribute metadata needed later to produce
     * tuples from raw C strings
     */
    attinmeta = TupleDescGetAttInMetadata(tupdesc);
    funcctx->attinmeta = attinmeta;

    POSTGIS_DEBUG(1, "lwt_GetFaceEdges calling SPI_finish");

    MemoryContextSwitchTo(oldcontext);

    SPI_finish();
  }

  /* stuff done on every call of the function */
  funcctx = SRF_PERCALL_SETUP();

  /* get state */
  state = funcctx->user_fctx;

  if ( state->curr == state->nelems )
  {
    SRF_RETURN_DONE(funcctx);
  }

  if ( snprintf(values[0], 32, "%d", state->curr+1) >= 32 )
  {
    lwerror("Face edge sequence number does not fit 32 chars ?!: %d",
            state->curr+1);
  }
  if ( snprintf(values[1], 32, "%" LWTFMT_ELEMID,
                state->elems[state->curr]) >= 32 )
  {
    lwerror("Signed edge identifier does not fit 32 chars ?!: %"
            LWTFMT_ELEMID, state->elems[state->curr]);
  }

  POSTGIS_DEBUGF(1, "ST_GetFaceEdges: cur:%d, val0:%s, val1:%s",
                 state->curr, values[0], values[1]);

  tuple = BuildTupleFromCStrings(funcctx->attinmeta, values);
  result = HeapTupleGetDatum(tuple);
  state->curr++;

  SRF_RETURN_NEXT(funcctx, result);
}

/*  ST_ChangeEdgeGeom(atopology, anedge, acurve) */
Datum ST_ChangeEdgeGeom(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(ST_ChangeEdgeGeom);
Datum ST_ChangeEdgeGeom(PG_FUNCTION_ARGS)
{
  text* toponame_text;
  char buf[64];
  char* toponame;
  int ret;
  LWT_ELEMID edge_id;
  GSERIALIZED *geom;
  LWGEOM *lwgeom;
  LWLINE *line;
  LWT_TOPOLOGY *topo;

  if ( PG_ARGISNULL(0) || PG_ARGISNULL(1) || PG_ARGISNULL(2) )
  {
    lwpgerror("SQL/MM Spatial exception - null argument");
    PG_RETURN_NULL();
  }

  toponame_text = PG_GETARG_TEXT_P(0);
  toponame = text_to_cstring(toponame_text);
  PG_FREE_IF_COPY(toponame_text, 0);

  edge_id = PG_GETARG_INT32(1) ;

  geom = PG_GETARG_GSERIALIZED_P(2);
  lwgeom = lwgeom_from_gserialized(geom);
  line = lwgeom_as_lwline(lwgeom);
  if ( ! line )
  {
    lwgeom_free(lwgeom);
    PG_FREE_IF_COPY(geom, 2);
    lwpgerror("ST_ChangeEdgeGeom third argument must be a line geometry");
    PG_RETURN_NULL();
  }

  if ( SPI_OK_CONNECT != SPI_connect() )
  {
    lwpgerror("Could not connect to SPI");
    PG_RETURN_NULL();
  }

  topo = lwt_LoadTopology(be_iface, toponame);
  pfree(toponame);
  if ( ! topo )
  {
    /* should never reach this point, as lwerror would raise an exception */
    SPI_finish();
    PG_RETURN_NULL();
  }

  POSTGIS_DEBUG(1, "Calling lwt_ChangeEdgeGeom");
  ret = lwt_ChangeEdgeGeom(topo, edge_id, line);
  POSTGIS_DEBUG(1, "lwt_ChangeEdgeGeom returned");
  lwgeom_free(lwgeom);
  PG_FREE_IF_COPY(geom, 2);
  lwt_FreeTopology(topo);

  if ( ret == -1 )
  {
    /* should never reach this point, as lwerror would raise an exception */
    SPI_finish();
    PG_RETURN_NULL();
  }

  SPI_finish();

  if ( snprintf(buf, 64, "Edge %" LWTFMT_ELEMID " changed", edge_id) >= 64 )
  {
    buf[63] = '\0';
  }
  PG_RETURN_TEXT_P(cstring_to_text(buf));
}

/*  ST_RemoveIsoNode(atopology, anode) */
Datum ST_RemoveIsoNode(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(ST_RemoveIsoNode);
Datum ST_RemoveIsoNode(PG_FUNCTION_ARGS)
{
  text* toponame_text;
  char buf[64];
  char* toponame;
  int ret;
  LWT_ELEMID node_id;
  LWT_TOPOLOGY *topo;

  if ( PG_ARGISNULL(0) || PG_ARGISNULL(1) )
  {
    lwpgerror("SQL/MM Spatial exception - null argument");
    PG_RETURN_NULL();
  }

  toponame_text = PG_GETARG_TEXT_P(0);
  toponame = text_to_cstring(toponame_text);
  PG_FREE_IF_COPY(toponame_text, 0);

  node_id = PG_GETARG_INT32(1) ;

  if ( SPI_OK_CONNECT != SPI_connect() )
  {
    lwpgerror("Could not connect to SPI");
    PG_RETURN_NULL();
  }

  topo = lwt_LoadTopology(be_iface, toponame);
  pfree(toponame);
  if ( ! topo )
  {
    /* should never reach this point, as lwerror would raise an exception */
    SPI_finish();
    PG_RETURN_NULL();
  }

  POSTGIS_DEBUG(1, "Calling lwt_RemoveIsoNode");
  ret = lwt_RemoveIsoNode(topo, node_id);
  POSTGIS_DEBUG(1, "lwt_RemoveIsoNode returned");
  lwt_FreeTopology(topo);

  if ( ret == -1 )
  {
    /* should never reach this point, as lwerror would raise an exception */
    SPI_finish();
    PG_RETURN_NULL();
  }

  /* TODO: check if any TopoGeometry exists including this point in
   * its definition ! */

  SPI_finish();

  if ( snprintf(buf, 64, "Isolated node %" LWTFMT_ELEMID
                " removed", node_id) >= 64 )
  {
    buf[63] = '\0';
  }
  PG_RETURN_TEXT_P(cstring_to_text(buf));
}

/*  ST_RemIsoEdge(atopology, anedge) */
Datum ST_RemIsoEdge(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(ST_RemIsoEdge);
Datum ST_RemIsoEdge(PG_FUNCTION_ARGS)
{
  text* toponame_text;
  char buf[64];
  char* toponame;
  int ret;
  LWT_ELEMID node_id;
  LWT_TOPOLOGY *topo;

  if ( PG_ARGISNULL(0) || PG_ARGISNULL(1) )
  {
    lwpgerror("SQL/MM Spatial exception - null argument");
    PG_RETURN_NULL();
  }

  toponame_text = PG_GETARG_TEXT_P(0);
  toponame = text_to_cstring(toponame_text);
  PG_FREE_IF_COPY(toponame_text, 0);

  node_id = PG_GETARG_INT32(1) ;

  if ( SPI_OK_CONNECT != SPI_connect() )
  {
    lwpgerror("Could not connect to SPI");
    PG_RETURN_NULL();
  }

  topo = lwt_LoadTopology(be_iface, toponame);
  pfree(toponame);
  if ( ! topo )
  {
    /* should never reach this point, as lwerror would raise an exception */
    SPI_finish();
    PG_RETURN_NULL();
  }

  POSTGIS_DEBUG(1, "Calling lwt_RemIsoEdge");
  ret = lwt_RemIsoEdge(topo, node_id);
  POSTGIS_DEBUG(1, "lwt_RemIsoEdge returned");
  lwt_FreeTopology(topo);

  if ( ret == -1 )
  {
    /* should never reach this point, as lwerror would raise an exception */
    SPI_finish();
    PG_RETURN_NULL();
  }

  /* TODO: check if any TopoGeometry exists including this point in
   * its definition ! */

  SPI_finish();

  if ( snprintf(buf, 64, "Isolated edge %" LWTFMT_ELEMID
                " removed", node_id) >= 64 )
  {
    buf[63] = '\0';
  }
  PG_RETURN_TEXT_P(cstring_to_text(buf));
}

/*  ST_MoveIsoNode(atopology, anode, apoint) */
Datum ST_MoveIsoNode(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(ST_MoveIsoNode);
Datum ST_MoveIsoNode(PG_FUNCTION_ARGS)
{
  text* toponame_text;
  char buf[64];
  char* toponame;
  int ret;
  LWT_ELEMID node_id;
  GSERIALIZED *geom;
  LWGEOM *lwgeom;
  LWPOINT *pt;
  LWT_TOPOLOGY *topo;
  POINT2D p;

  if ( PG_ARGISNULL(0) || PG_ARGISNULL(1) || PG_ARGISNULL(2) )
  {
    lwpgerror("SQL/MM Spatial exception - null argument");
    PG_RETURN_NULL();
  }

  toponame_text = PG_GETARG_TEXT_P(0);
  toponame = text_to_cstring(toponame_text);
  PG_FREE_IF_COPY(toponame_text, 0);

  node_id = PG_GETARG_INT32(1) ;

  geom = PG_GETARG_GSERIALIZED_P(2);
  lwgeom = lwgeom_from_gserialized(geom);
  pt = lwgeom_as_lwpoint(lwgeom);
  if ( ! pt )
  {
    lwgeom_free(lwgeom);
    PG_FREE_IF_COPY(geom, 2);
    lwpgerror("SQL/MM Spatial exception - invalid point");
    PG_RETURN_NULL();
  }

  if ( ! getPoint2d_p(pt->point, 0, &p) )
  {
    /* Do not let empty points in, see
     * https://trac.osgeo.org/postgis/ticket/3234
     */
    lwpgerror("SQL/MM Spatial exception - empty point");
    PG_RETURN_NULL();
  }

  /* TODO: check point for NaN values ? */

  if ( SPI_OK_CONNECT != SPI_connect() )
  {
    lwpgerror("Could not connect to SPI");
    PG_RETURN_NULL();
  }

  topo = lwt_LoadTopology(be_iface, toponame);
  pfree(toponame);
  if ( ! topo )
  {
    /* should never reach this point, as lwerror would raise an exception */
    SPI_finish();
    PG_RETURN_NULL();
  }

  POSTGIS_DEBUG(1, "Calling lwt_MoveIsoNode");
  ret = lwt_MoveIsoNode(topo, node_id, pt);
  POSTGIS_DEBUG(1, "lwt_MoveIsoNode returned");
  lwgeom_free(lwgeom);
  PG_FREE_IF_COPY(geom, 2);
  lwt_FreeTopology(topo);

  if ( ret == -1 )
  {
    /* should never reach this point, as lwerror would raise an exception */
    SPI_finish();
    PG_RETURN_NULL();
  }

  SPI_finish();

  if ( snprintf(buf, 64, "Isolated Node %" LWTFMT_ELEMID
                " moved to location %g,%g",
                node_id, p.x, p.y) >= 64 )
  {
    buf[63] = '\0';
  }
  PG_RETURN_TEXT_P(cstring_to_text(buf));
}

/*  ST_RemEdgeModFace(atopology, anedge) */
Datum ST_RemEdgeModFace(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(ST_RemEdgeModFace);
Datum ST_RemEdgeModFace(PG_FUNCTION_ARGS)
{
  text* toponame_text;
  char* toponame;
  int ret;
  LWT_ELEMID node_id;
  LWT_TOPOLOGY *topo;

  if ( PG_ARGISNULL(0) || PG_ARGISNULL(1) )
  {
    lwpgerror("SQL/MM Spatial exception - null argument");
    PG_RETURN_NULL();
  }

  toponame_text = PG_GETARG_TEXT_P(0);
  toponame = text_to_cstring(toponame_text);
  PG_FREE_IF_COPY(toponame_text, 0);

  node_id = PG_GETARG_INT32(1) ;

  if ( SPI_OK_CONNECT != SPI_connect() )
  {
    lwpgerror("Could not connect to SPI");
    PG_RETURN_NULL();
  }

  topo = lwt_LoadTopology(be_iface, toponame);
  pfree(toponame);
  if ( ! topo )
  {
    /* should never reach this point, as lwerror would raise an exception */
    SPI_finish();
    PG_RETURN_NULL();
  }

  POSTGIS_DEBUG(1, "Calling lwt_RemEdgeModFace");
  ret = lwt_RemEdgeModFace(topo, node_id);
  POSTGIS_DEBUG(1, "lwt_RemEdgeModFace returned");
  lwt_FreeTopology(topo);

  if ( ret == -1 )
  {
    /* should never reach this point, as lwerror would raise an exception */
    SPI_finish();
    PG_RETURN_NULL();
  }

  SPI_finish();

  PG_RETURN_INT32(ret);
}

/*  ST_RemEdgeNewFace(atopology, anedge) */
Datum ST_RemEdgeNewFace(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(ST_RemEdgeNewFace);
Datum ST_RemEdgeNewFace(PG_FUNCTION_ARGS)
{
  text* toponame_text;
  char* toponame;
  int ret;
  LWT_ELEMID node_id;
  LWT_TOPOLOGY *topo;

  if ( PG_ARGISNULL(0) || PG_ARGISNULL(1) )
  {
    lwpgerror("SQL/MM Spatial exception - null argument");
    PG_RETURN_NULL();
  }

  toponame_text = PG_GETARG_TEXT_P(0);
  toponame = text_to_cstring(toponame_text);
  PG_FREE_IF_COPY(toponame_text, 0);

  node_id = PG_GETARG_INT32(1) ;

  if ( SPI_OK_CONNECT != SPI_connect() )
  {
    lwpgerror("Could not connect to SPI");
    PG_RETURN_NULL();
  }

  topo = lwt_LoadTopology(be_iface, toponame);
  pfree(toponame);
  if ( ! topo )
  {
    /* should never reach this point, as lwerror would raise an exception */
    SPI_finish();
    PG_RETURN_NULL();
  }

  POSTGIS_DEBUG(1, "Calling lwt_RemEdgeNewFace");
  ret = lwt_RemEdgeNewFace(topo, node_id);
  POSTGIS_DEBUG(1, "lwt_RemEdgeNewFace returned");
  lwt_FreeTopology(topo);
  SPI_finish();

  if ( ret <= 0 )
  {
    /* error or no face created */
    PG_RETURN_NULL();
  }

  PG_RETURN_INT32(ret);
}

/*  ST_ModEdgeHeal(atopology, anedge, anotheredge) */
Datum ST_ModEdgeHeal(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(ST_ModEdgeHeal);
Datum ST_ModEdgeHeal(PG_FUNCTION_ARGS)
{
  text* toponame_text;
  char* toponame;
  int ret;
  LWT_ELEMID eid1, eid2;
  LWT_TOPOLOGY *topo;

  if ( PG_ARGISNULL(0) || PG_ARGISNULL(1) || PG_ARGISNULL(2) )
  {
    lwpgerror("SQL/MM Spatial exception - null argument");
    PG_RETURN_NULL();
  }

  toponame_text = PG_GETARG_TEXT_P(0);
  toponame = text_to_cstring(toponame_text);
  PG_FREE_IF_COPY(toponame_text, 0);

  eid1 = PG_GETARG_INT32(1) ;
  eid2 = PG_GETARG_INT32(2) ;

  if ( SPI_OK_CONNECT != SPI_connect() )
  {
    lwpgerror("Could not connect to SPI");
    PG_RETURN_NULL();
  }

  topo = lwt_LoadTopology(be_iface, toponame);
  pfree(toponame);
  if ( ! topo )
  {
    /* should never reach this point, as lwerror would raise an exception */
    SPI_finish();
    PG_RETURN_NULL();
  }

  POSTGIS_DEBUG(1, "Calling lwt_ModEdgeHeal");
  ret = lwt_ModEdgeHeal(topo, eid1, eid2);
  POSTGIS_DEBUG(1, "lwt_ModEdgeHeal returned");
  lwt_FreeTopology(topo);
  SPI_finish();

  if ( ret <= 0 )
  {
    /* error, should have sent message already */
    PG_RETURN_NULL();
  }

  PG_RETURN_INT32(ret);
}

/*  ST_NewEdgeHeal(atopology, anedge, anotheredge) */
Datum ST_NewEdgeHeal(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(ST_NewEdgeHeal);
Datum ST_NewEdgeHeal(PG_FUNCTION_ARGS)
{
  text* toponame_text;
  char* toponame;
  int ret;
  LWT_ELEMID eid1, eid2;
  LWT_TOPOLOGY *topo;

  if ( PG_ARGISNULL(0) || PG_ARGISNULL(1) || PG_ARGISNULL(2) )
  {
    lwpgerror("SQL/MM Spatial exception - null argument");
    PG_RETURN_NULL();
  }

  toponame_text = PG_GETARG_TEXT_P(0);
  toponame = text_to_cstring(toponame_text);
  PG_FREE_IF_COPY(toponame_text, 0);

  eid1 = PG_GETARG_INT32(1) ;
  eid2 = PG_GETARG_INT32(2) ;

  if ( SPI_OK_CONNECT != SPI_connect() )
  {
    lwpgerror("Could not connect to SPI");
    PG_RETURN_NULL();
  }

  topo = lwt_LoadTopology(be_iface, toponame);
  pfree(toponame);
  if ( ! topo )
  {
    /* should never reach this point, as lwerror would raise an exception */
    SPI_finish();
    PG_RETURN_NULL();
  }

  POSTGIS_DEBUG(1, "Calling lwt_NewEdgeHeal");
  ret = lwt_NewEdgeHeal(topo, eid1, eid2);
  POSTGIS_DEBUG(1, "lwt_NewEdgeHeal returned");
  lwt_FreeTopology(topo);
  SPI_finish();

  if ( ret <= 0 )
  {
    /* error, should have sent message already */
    PG_RETURN_NULL();
  }

  PG_RETURN_INT32(ret);
}

/*  GetNodeByPoint(atopology, point, tolerance) */
Datum GetNodeByPoint(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(GetNodeByPoint);
Datum GetNodeByPoint(PG_FUNCTION_ARGS)
{
  text* toponame_text;
  char* toponame;
  double tol;
  LWT_ELEMID node_id;
  GSERIALIZED *geom;
  LWGEOM *lwgeom;
  LWPOINT *pt;
  LWT_TOPOLOGY *topo;

  toponame_text = PG_GETARG_TEXT_P(0);
  toponame = text_to_cstring(toponame_text);
  PG_FREE_IF_COPY(toponame_text, 0);

  geom = PG_GETARG_GSERIALIZED_P(1);
  lwgeom = lwgeom_from_gserialized(geom);
  pt = lwgeom_as_lwpoint(lwgeom);
  if ( ! pt )
  {
    lwgeom_free(lwgeom);
    PG_FREE_IF_COPY(geom, 1);
    lwpgerror("Node geometry must be a point");
    PG_RETURN_NULL();
  }

  tol = PG_GETARG_FLOAT8(2);
  if ( tol < 0 )
  {
    PG_FREE_IF_COPY(geom, 1);
    lwpgerror("Tolerance must be >=0");
    PG_RETURN_NULL();
  }

  if ( SPI_OK_CONNECT != SPI_connect() )
  {
    lwpgerror("Could not connect to SPI");
    PG_RETURN_NULL();
  }

  topo = lwt_LoadTopology(be_iface, toponame);
  pfree(toponame);
  if ( ! topo )
  {
    /* should never reach this point, as lwerror would raise an exception */
    SPI_finish();
    PG_RETURN_NULL();
  }

  POSTGIS_DEBUG(1, "Calling lwt_GetNodeByPoint");
  node_id = lwt_GetNodeByPoint(topo, pt, tol);
  POSTGIS_DEBUG(1, "lwt_GetNodeByPoint returned");
  lwgeom_free(lwgeom);
  PG_FREE_IF_COPY(geom, 1);
  lwt_FreeTopology(topo);

  if ( node_id == -1 )
  {
    /* should never reach this point, as lwerror would raise an exception */
    SPI_finish();
    PG_RETURN_NULL();
  }

  SPI_finish();
  PG_RETURN_INT32(node_id);
}

/*  GetEdgeByPoint(atopology, point, tolerance) */
Datum GetEdgeByPoint(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(GetEdgeByPoint);
Datum GetEdgeByPoint(PG_FUNCTION_ARGS)
{
  text* toponame_text;
  char* toponame;
  double tol;
  LWT_ELEMID node_id;
  GSERIALIZED *geom;
  LWGEOM *lwgeom;
  LWPOINT *pt;
  LWT_TOPOLOGY *topo;

  toponame_text = PG_GETARG_TEXT_P(0);
  toponame = text_to_cstring(toponame_text);
  PG_FREE_IF_COPY(toponame_text, 0);

  geom = PG_GETARG_GSERIALIZED_P(1);
  lwgeom = lwgeom_from_gserialized(geom);
  pt = lwgeom_as_lwpoint(lwgeom);
  if ( ! pt )
  {
    lwgeom_free(lwgeom);
    PG_FREE_IF_COPY(geom, 1);
    lwpgerror("Node geometry must be a point");
    PG_RETURN_NULL();
  }

  tol = PG_GETARG_FLOAT8(2);
  if ( tol < 0 )
  {
    PG_FREE_IF_COPY(geom, 1);
    lwpgerror("Tolerance must be >=0");
    PG_RETURN_NULL();
  }

  if ( SPI_OK_CONNECT != SPI_connect() )
  {
    lwpgerror("Could not connect to SPI");
    PG_RETURN_NULL();
  }

  topo = lwt_LoadTopology(be_iface, toponame);
  pfree(toponame);
  if ( ! topo )
  {
    /* should never reach this point, as lwerror would raise an exception */
    SPI_finish();
    PG_RETURN_NULL();
  }

  POSTGIS_DEBUG(1, "Calling lwt_GetEdgeByPoint");
  node_id = lwt_GetEdgeByPoint(topo, pt, tol);
  POSTGIS_DEBUG(1, "lwt_GetEdgeByPoint returned");
  lwgeom_free(lwgeom);
  PG_FREE_IF_COPY(geom, 1);
  lwt_FreeTopology(topo);

  if ( node_id == -1 )
  {
    /* should never reach this point, as lwerror would raise an exception */
    SPI_finish();
    PG_RETURN_NULL();
  }

  SPI_finish();
  PG_RETURN_INT32(node_id);
}

/*  GetFaceByPoint(atopology, point, tolerance) */
Datum GetFaceByPoint(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(GetFaceByPoint);
Datum GetFaceByPoint(PG_FUNCTION_ARGS)
{
  text* toponame_text;
  char* toponame;
  double tol;
  LWT_ELEMID node_id;
  GSERIALIZED *geom;
  LWGEOM *lwgeom;
  LWPOINT *pt;
  LWT_TOPOLOGY *topo;

  toponame_text = PG_GETARG_TEXT_P(0);
  toponame = text_to_cstring(toponame_text);
  PG_FREE_IF_COPY(toponame_text, 0);

  geom = PG_GETARG_GSERIALIZED_P(1);
  lwgeom = lwgeom_from_gserialized(geom);
  pt = lwgeom_as_lwpoint(lwgeom);
  if ( ! pt )
  {
    lwgeom_free(lwgeom);
    PG_FREE_IF_COPY(geom, 1);
    lwpgerror("Node geometry must be a point");
    PG_RETURN_NULL();
  }

  tol = PG_GETARG_FLOAT8(2);
  if ( tol < 0 )
  {
    PG_FREE_IF_COPY(geom, 1);
    lwpgerror("Tolerance must be >=0");
    PG_RETURN_NULL();
  }

  if ( SPI_OK_CONNECT != SPI_connect() )
  {
    lwpgerror("Could not connect to SPI");
    PG_RETURN_NULL();
  }

  topo = lwt_LoadTopology(be_iface, toponame);
  pfree(toponame);
  if ( ! topo )
  {
    /* should never reach this point, as lwerror would raise an exception */
    SPI_finish();
    PG_RETURN_NULL();
  }

  POSTGIS_DEBUG(1, "Calling lwt_GetFaceByPoint");
  node_id = lwt_GetFaceByPoint(topo, pt, tol);
  POSTGIS_DEBUG(1, "lwt_GetFaceByPoint returned");
  lwgeom_free(lwgeom);
  PG_FREE_IF_COPY(geom, 1);
  lwt_FreeTopology(topo);

  if ( node_id == -1 )
  {
    /* should never reach this point, as lwerror would raise an exception */
    SPI_finish();
    PG_RETURN_NULL();
  }

  SPI_finish();
  PG_RETURN_INT32(node_id);
}

/*  TopoGeo_AddPoint(atopology, point, tolerance) */
Datum TopoGeo_AddPoint(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(TopoGeo_AddPoint);
Datum TopoGeo_AddPoint(PG_FUNCTION_ARGS)
{
  text* toponame_text;
  char* toponame;
  double tol;
  LWT_ELEMID node_id;
  GSERIALIZED *geom;
  LWGEOM *lwgeom;
  LWPOINT *pt;
  LWT_TOPOLOGY *topo;

  if ( PG_ARGISNULL(0) || PG_ARGISNULL(1) || PG_ARGISNULL(2) )
  {
    lwpgerror("SQL/MM Spatial exception - null argument");
    PG_RETURN_NULL();
  }

  toponame_text = PG_GETARG_TEXT_P(0);
  toponame = text_to_cstring(toponame_text);
  PG_FREE_IF_COPY(toponame_text, 0);

  geom = PG_GETARG_GSERIALIZED_P(1);
  lwgeom = lwgeom_from_gserialized(geom);
  pt = lwgeom_as_lwpoint(lwgeom);
  if ( ! pt )
  {
    {
      char buf[32];
      _lwtype_upper_name(lwgeom_get_type(lwgeom), buf, 32);
      lwgeom_free(lwgeom);
      PG_FREE_IF_COPY(geom, 1);
      lwpgerror("Invalid geometry type (%s) passed to TopoGeo_AddPoint"
                ", expected POINT", buf );
      PG_RETURN_NULL();
    }
  }

  tol = PG_GETARG_FLOAT8(2);
  if ( tol < 0 )
  {
    PG_FREE_IF_COPY(geom, 1);
    lwpgerror("Tolerance must be >=0");
    PG_RETURN_NULL();
  }

  if ( SPI_OK_CONNECT != SPI_connect() )
  {
    lwpgerror("Could not connect to SPI");
    PG_RETURN_NULL();
  }

  {
    int pre = be_data.topoLoadFailMessageFlavor;
    be_data.topoLoadFailMessageFlavor = 1;
    topo = lwt_LoadTopology(be_iface, toponame);
    be_data.topoLoadFailMessageFlavor = pre;
  }
  pfree(toponame);
  if ( ! topo )
  {
    /* should never reach this point, as lwerror would raise an exception */
    SPI_finish();
    PG_RETURN_NULL();
  }

  POSTGIS_DEBUG(1, "Calling lwt_AddPoint");
  node_id = lwt_AddPoint(topo, pt, tol);
  POSTGIS_DEBUG(1, "lwt_AddPoint returned");
  lwgeom_free(lwgeom);
  PG_FREE_IF_COPY(geom, 1);
  lwt_FreeTopology(topo);

  if ( node_id == -1 )
  {
    /* should never reach this point, as lwerror would raise an exception */
    SPI_finish();
    PG_RETURN_NULL();
  }

  SPI_finish();
  PG_RETURN_INT32(node_id);
}

/*  TopoGeo_AddLinestring(atopology, point, tolerance) */
Datum TopoGeo_AddLinestring(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(TopoGeo_AddLinestring);
Datum TopoGeo_AddLinestring(PG_FUNCTION_ARGS)
{
  text* toponame_text;
  char* toponame;
  double tol;
  LWT_ELEMID *elems;
  int nelems;
  GSERIALIZED *geom;
  LWGEOM *lwgeom;
  LWLINE *ln;
  LWT_TOPOLOGY *topo;
  FuncCallContext *funcctx;
  MemoryContext oldcontext, newcontext;
  FACEEDGESSTATE *state;
  Datum result;
  LWT_ELEMID id;

  if (SRF_IS_FIRSTCALL())
  {
    POSTGIS_DEBUG(1, "TopoGeo_AddLinestring first call");
    funcctx = SRF_FIRSTCALL_INIT();
    newcontext = funcctx->multi_call_memory_ctx;

    if ( PG_ARGISNULL(0) || PG_ARGISNULL(1) || PG_ARGISNULL(2) )
    {
      lwpgerror("SQL/MM Spatial exception - null argument");
      PG_RETURN_NULL();
    }

    toponame_text = PG_GETARG_TEXT_P(0);
    toponame = text_to_cstring(toponame_text);
    PG_FREE_IF_COPY(toponame_text, 0);

    geom = PG_GETARG_GSERIALIZED_P(1);
    lwgeom = lwgeom_from_gserialized(geom);
    ln = lwgeom_as_lwline(lwgeom);
    if ( ! ln )
    {
      {
        char buf[32];
        _lwtype_upper_name(lwgeom_get_type(lwgeom), buf, 32);
        lwgeom_free(lwgeom);
        PG_FREE_IF_COPY(geom, 1);
        lwpgerror("Invalid geometry type (%s) passed to "
                  "TopoGeo_AddLinestring, expected LINESTRING", buf);
        PG_RETURN_NULL();
      }
    }

    tol = PG_GETARG_FLOAT8(2);
    if ( tol < 0 )
    {
      PG_FREE_IF_COPY(geom, 1);
      lwpgerror("Tolerance must be >=0");
      PG_RETURN_NULL();
    }

    if ( SPI_OK_CONNECT != SPI_connect() )
    {
      lwpgerror("Could not connect to SPI");
      PG_RETURN_NULL();
    }

    {
      int pre = be_data.topoLoadFailMessageFlavor;
      be_data.topoLoadFailMessageFlavor = 1;
      topo = lwt_LoadTopology(be_iface, toponame);
      be_data.topoLoadFailMessageFlavor = pre;
    }
    oldcontext = MemoryContextSwitchTo( newcontext );
    pfree(toponame);
    if ( ! topo )
    {
      /* should never reach this point, as lwerror would raise an exception */
      SPI_finish();
      PG_RETURN_NULL();
    }

    POSTGIS_DEBUG(1, "Calling lwt_AddLine");
    elems = lwt_AddLine(topo, ln, tol, &nelems);
    POSTGIS_DEBUG(1, "lwt_AddLine returned");
    lwgeom_free(lwgeom);
    PG_FREE_IF_COPY(geom, 1);
    lwt_FreeTopology(topo);

    if ( nelems < 0 )
    {
      /* should never reach this point, as lwerror would raise an exception */
      SPI_finish();
      PG_RETURN_NULL();
    }

    state = lwalloc(sizeof(FACEEDGESSTATE));
    state->elems = elems;
    state->nelems = nelems;
    state->curr = 0;
    funcctx->user_fctx = state;

    POSTGIS_DEBUG(1, "TopoGeo_AddLinestring calling SPI_finish");

    MemoryContextSwitchTo(oldcontext);

    SPI_finish();
  }

  POSTGIS_DEBUG(1, "Per-call invocation");

  /* stuff done on every call of the function */
  funcctx = SRF_PERCALL_SETUP();

  /* get state */
  state = funcctx->user_fctx;

  if ( state->curr == state->nelems )
  {
    POSTGIS_DEBUG(1, "We're done, cleaning up all");
    SRF_RETURN_DONE(funcctx);
  }

  id = state->elems[state->curr++];
  POSTGIS_DEBUGF(1, "TopoGeo_AddLinestring: cur:%d, val:%" LWTFMT_ELEMID,
                 state->curr-1, id);

  result = Int32GetDatum((int32)id);

  SRF_RETURN_NEXT(funcctx, result);
}

/*  TopoGeo_AddPolygon(atopology, poly, tolerance) */
Datum TopoGeo_AddPolygon(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(TopoGeo_AddPolygon);
Datum TopoGeo_AddPolygon(PG_FUNCTION_ARGS)
{
  text* toponame_text;
  char* toponame;
  double tol;
  LWT_ELEMID *elems;
  int nelems;
  GSERIALIZED *geom;
  LWGEOM *lwgeom;
  LWPOLY *pol;
  LWT_TOPOLOGY *topo;
  FuncCallContext *funcctx;
  MemoryContext oldcontext, newcontext;
  FACEEDGESSTATE *state;
  Datum result;
  LWT_ELEMID id;

  if (SRF_IS_FIRSTCALL())
  {
    POSTGIS_DEBUG(1, "TopoGeo_AddPolygon first call");
    funcctx = SRF_FIRSTCALL_INIT();
    newcontext = funcctx->multi_call_memory_ctx;

    if ( PG_ARGISNULL(0) || PG_ARGISNULL(1) || PG_ARGISNULL(2) )
    {
      lwpgerror("SQL/MM Spatial exception - null argument");
      PG_RETURN_NULL();
    }

    toponame_text = PG_GETARG_TEXT_P(0);
    toponame = text_to_cstring(toponame_text);
    PG_FREE_IF_COPY(toponame_text, 0);

    geom = PG_GETARG_GSERIALIZED_P(1);
    lwgeom = lwgeom_from_gserialized(geom);
    pol = lwgeom_as_lwpoly(lwgeom);
    if ( ! pol )
    {
      {
        char buf[32];
        _lwtype_upper_name(lwgeom_get_type(lwgeom), buf, 32);
        lwgeom_free(lwgeom);
        PG_FREE_IF_COPY(geom, 1);
        lwpgerror("Invalid geometry type (%s) passed to "
                  "TopoGeo_AddPolygon, expected POLYGON", buf);
        PG_RETURN_NULL();
      }
    }

    tol = PG_GETARG_FLOAT8(2);
    if ( tol < 0 )
    {
      PG_FREE_IF_COPY(geom, 1);
      lwpgerror("Tolerance must be >=0");
      PG_RETURN_NULL();
    }

    if ( SPI_OK_CONNECT != SPI_connect() )
    {
      lwpgerror("Could not connect to SPI");
      PG_RETURN_NULL();
    }

    {
      int pre = be_data.topoLoadFailMessageFlavor;
      be_data.topoLoadFailMessageFlavor = 1;
      topo = lwt_LoadTopology(be_iface, toponame);
      be_data.topoLoadFailMessageFlavor = pre;
    }
    oldcontext = MemoryContextSwitchTo( newcontext );
    pfree(toponame);
    if ( ! topo )
    {
      /* should never reach this point, as lwerror would raise an exception */
      SPI_finish();
      PG_RETURN_NULL();
    }

    POSTGIS_DEBUG(1, "Calling lwt_AddPolygon");
    elems = lwt_AddPolygon(topo, pol, tol, &nelems);
    POSTGIS_DEBUG(1, "lwt_AddPolygon returned");
    lwgeom_free(lwgeom);
    PG_FREE_IF_COPY(geom, 1);
    lwt_FreeTopology(topo);

    if ( nelems < 0 )
    {
      /* should never reach this point, as lwerror would raise an exception */
      SPI_finish();
      PG_RETURN_NULL();
    }

    state = lwalloc(sizeof(FACEEDGESSTATE));
    state->elems = elems;
    state->nelems = nelems;
    state->curr = 0;
    funcctx->user_fctx = state;

    POSTGIS_DEBUG(1, "TopoGeo_AddPolygon calling SPI_finish");

    MemoryContextSwitchTo(oldcontext);

    SPI_finish();
  }

  POSTGIS_DEBUG(1, "Per-call invocation");

  /* stuff done on every call of the function */
  funcctx = SRF_PERCALL_SETUP();

  /* get state */
  state = funcctx->user_fctx;

  if ( state->curr == state->nelems )
  {
    POSTGIS_DEBUG(1, "We're done, cleaning up all");
    SRF_RETURN_DONE(funcctx);
  }

  id = state->elems[state->curr++];
  POSTGIS_DEBUGF(1, "TopoGeo_AddPolygon: cur:%d, val:%" LWTFMT_ELEMID,
                 state->curr-1, id);

  result = Int32GetDatum((int32)id);

  SRF_RETURN_NEXT(funcctx, result);
}

/*  GetRingEdges(atopology, anedge, maxedges default null) */
Datum GetRingEdges(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(GetRingEdges);
Datum GetRingEdges(PG_FUNCTION_ARGS)
{
  text* toponame_text;
  char* toponame;
  LWT_ELEMID edge_id;
  int maxedges = 0;
  uint64_t nelems;
  LWT_ELEMID *elems;
  LWT_BE_TOPOLOGY *topo;
  FuncCallContext *funcctx;
  MemoryContext oldcontext, newcontext;
  FACEEDGESSTATE *state;
  Datum result;
  HeapTuple tuple;
  Datum ret[2];
  bool isnull[2] = {0,0}; /* needed to say neither value is null */

  if (SRF_IS_FIRSTCALL())
  {
    POSTGIS_DEBUG(1, "GetRingEdges first call");
    funcctx = SRF_FIRSTCALL_INIT();
    newcontext = funcctx->multi_call_memory_ctx;


    if ( PG_ARGISNULL(0) )
    {
      lwpgerror("GetRingEdges: topology name cannot be null");
      PG_RETURN_NULL();
    }
    toponame_text = PG_GETARG_TEXT_P(0);
    toponame = text_to_cstring(toponame_text);
    PG_FREE_IF_COPY(toponame_text, 0);

    if ( PG_ARGISNULL(1) )
    {
      lwpgerror("GetRingEdges: edge id cannot be null");
      PG_RETURN_NULL();
    }
    edge_id = PG_GETARG_INT32(1) ;

    if ( ! PG_ARGISNULL(2) )
    {
      maxedges = PG_GETARG_INT32(2) ;
    }

    if ( SPI_OK_CONNECT != SPI_connect() )
    {
      lwpgerror("Could not connect to SPI");
      PG_RETURN_NULL();
    }

    {
      int pre = be_data.topoLoadFailMessageFlavor;
      be_data.topoLoadFailMessageFlavor = 1;
      topo = cb_loadTopologyByName(&be_data, toponame);
      be_data.topoLoadFailMessageFlavor = pre;
    }
    oldcontext = MemoryContextSwitchTo( newcontext );
    pfree(toponame);
    if ( ! topo )
    {
      /* should never reach this point, as lwerror would raise an exception */
      SPI_finish();
      PG_RETURN_NULL();
    }

    POSTGIS_DEBUG(1, "Calling cb_getRingEdges");
    elems = cb_getRingEdges(topo, edge_id, &nelems, maxedges);
    POSTGIS_DEBUG(1, "cb_getRingEdges returned");
    cb_freeTopology(topo);

    if ( ! elems )
    {
      /* should never reach this point, as lwerror would raise an exception */
      SPI_finish();
      PG_RETURN_NULL();
    }

    state = lwalloc(sizeof(FACEEDGESSTATE));
    state->elems = elems;
    state->nelems = nelems;
    state->curr = 0;
    funcctx->user_fctx = state;

    POSTGIS_DEBUG(1, "GetRingEdges calling SPI_finish");

    /*
     * Get tuple description for return type
     */
    get_call_result_type(fcinfo, 0, &funcctx->tuple_desc);
    BlessTupleDesc(funcctx->tuple_desc);

    MemoryContextSwitchTo(oldcontext);

    SPI_finish();
  }

  POSTGIS_DEBUG(1, "Per-call invocation");

  /* stuff done on every call of the function */
  funcctx = SRF_PERCALL_SETUP();

  /* get state */
  state = funcctx->user_fctx;

  if ( state->curr == state->nelems )
  {
    POSTGIS_DEBUG(1, "We're done, cleaning up all");
    SRF_RETURN_DONE(funcctx);
  }

  edge_id = state->elems[state->curr++];
  POSTGIS_DEBUGF(1, "GetRingEdges: cur:%d, val:%" LWTFMT_ELEMID,
                 state->curr-1, edge_id);


  ret[0] = Int32GetDatum(state->curr);
  ret[1] = Int32GetDatum(edge_id);
  tuple = heap_form_tuple(funcctx->tuple_desc, ret, isnull);
  result = HeapTupleGetDatum(tuple);

  SRF_RETURN_NEXT(funcctx, result);
}
