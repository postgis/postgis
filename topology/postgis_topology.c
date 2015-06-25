/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * Copyright (C) 2015 Sandro Santilli <strk@keybit.net>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "postgres.h"
#include "fmgr.h"
#include "utils/elog.h"
#include "utils/memutils.h" /* for TopMemoryContext */
#include "lib/stringinfo.h"
//#include "funcapi.h"
#include "executor/spi.h" /* this is what you need to work with SPI */

#include "../postgis_config.h"

#include "liblwgeom_topo.h"
#include "lwgeom_log.h"
#include "lwgeom_pg.h"

#include <stdarg.h>

#ifdef __GNUC__
# define GNU_PRINTF23 __attribute__ (( format(printf, 2, 3) ))
#else
# define GNU_PRINTF23
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
struct LWT_BE_DATA_T {
  char lastErrorMsg[MAXERRLEN];
};
LWT_BE_DATA be_data;

struct LWT_BE_TOPOLOGY_T {
  LWT_BE_DATA* be_data;
  char *name;
  int id;
  int srid;
  int precision;
};

/* utility funx */

static void cberror(const LWT_BE_DATA* be, const char *fmt, ...) GNU_PRINTF23;

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
  StringInfoData sqldata;
  StringInfo sql = &sqldata;
  Datum dat;
  bool isnull;
  LWT_BE_TOPOLOGY *topo;

  initStringInfo(sql);
  appendStringInfo(sql, "SELECT * FROM topology.topology "
                        "WHERE name = '%s'", name);
  spi_result = SPI_execute(sql->data, true, 0);
  if ( spi_result != SPI_OK_SELECT ) {
    pfree(sqldata.data);
		cberror(be, "unexpected return (%d) from query execution: %s", spi_result, sql->data);
	  return NULL;
  }
  if ( ! SPI_processed )
  {
    pfree(sqldata.data);
		//cberror(be, "no topology named '%s' was found", name);
		cberror(be, "SQL/MM Spatial exception - invalid topology name");
	  return NULL;
  }
  if ( SPI_processed > 1 )
  {
    pfree(sqldata.data);
		cberror(be, "multiple topologies named '%s' were found", name);
	  return NULL;
  }
  pfree(sqldata.data);

  topo = palloc(sizeof(LWT_BE_TOPOLOGY));
  topo->be_data = (LWT_BE_DATA *)be; /* const cast.. */
  topo->name = pstrdup(name);

  dat = SPI_getbinval(SPI_tuptable->vals[0], SPI_tuptable->tupdesc, 1, &isnull);
  if ( isnull ) {
		cberror(be, "Topology '%s' has null identifier", name);
    return NULL;
  }
  topo->id = DatumGetInt32(dat);

  topo->srid = 0; /* needed ? */
  topo->precision = 0; /* needed ? */

  lwpgnotice("cb_loadTopologyByName: topo '%s' has id %d", name, topo->id);

  return topo;
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

  if ( fields & LWT_COL_EDGE_EDGE_ID ) {
    appendStringInfoString(str, "edge_id");
    sep = ",";
  }
  if ( fields & LWT_COL_EDGE_START_NODE ) {
    appendStringInfo(str, "%sstart_node", sep);
    sep = ",";
  }
  if ( fields & LWT_COL_EDGE_END_NODE ) {
    appendStringInfo(str, "%send_node", sep);
    sep = ",";
  }
  if ( fields & LWT_COL_EDGE_FACE_LEFT ) {
    appendStringInfo(str, "%sleft_face", sep);
    sep = ",";
  }
  if ( fields & LWT_COL_EDGE_FACE_RIGHT ) {
    appendStringInfo(str, "%sright_face", sep);
    sep = ",";
  }
  if ( fields & LWT_COL_EDGE_NEXT_LEFT ) {
    appendStringInfo(str, "%snext_left_edge", sep);
    if ( fullEdgeData ) appendStringInfoString(str, ", abs_next_left_edge");
    sep = ",";
  }
  if ( fields & LWT_COL_EDGE_NEXT_RIGHT ) {
    appendStringInfo(str, "%snext_right_edge", sep);
    if ( fullEdgeData ) appendStringInfoString(str, ", abs_next_right_edge");
    sep = ",";
  }
  if ( fields & LWT_COL_EDGE_GEOM ) {
    appendStringInfo(str, "%sgeom", sep);
  }
}

/* Add edge values for an insert, in text form */
static void
addEdgeValues(StringInfo str, LWT_ISO_EDGE *edge, int fullEdgeData)
{
  size_t hexewkb_size;
  char *hexewkb;

  if ( edge->edge_id != -1 ) appendStringInfo(str, "(%lld", edge->edge_id);
  else appendStringInfoString(str, "(DEFAULT");

  appendStringInfo(str, ",%lld", edge->start_node);
  appendStringInfo(str, ",%lld", edge->end_node);
  appendStringInfo(str, ",%lld", edge->face_left);
  appendStringInfo(str, ",%lld", edge->face_right);
  appendStringInfo(str, ",%lld", edge->next_left);
  if ( fullEdgeData ) appendStringInfo(str, ",%lld", llabs(edge->next_left));
  appendStringInfo(str, ",%lld", edge->next_right);
  if ( fullEdgeData ) appendStringInfo(str, ",%lld", llabs(edge->next_right));

  if ( edge->geom ) {
    hexewkb = lwgeom_to_hexwkb(lwline_as_lwgeom(edge->geom),
                                WKB_EXTENDED, &hexewkb_size);
    appendStringInfo(str, ",'%s'::geometry)", hexewkb);
    lwfree(hexewkb);
  } else {
    appendStringInfoString(str, ",null)");
  }
}

enum UpdateType {
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
  size_t hexewkb_size;
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

  if ( fields & LWT_COL_EDGE_EDGE_ID ) {
    appendStringInfoString(str, "edge_id ");
    appendStringInfo(str, "%s %lld", op, edge->edge_id);
    sep = sep1;
  }
  if ( fields & LWT_COL_EDGE_START_NODE ) {
    appendStringInfo(str, "%sstart_node ", sep);
    appendStringInfo(str, "%s %lld", op, edge->start_node);
    sep = sep1;
  }
  if ( fields & LWT_COL_EDGE_END_NODE ) {
    appendStringInfo(str, "%send_node", sep);
    appendStringInfo(str, "%s %lld", op, edge->end_node);
    sep = sep1;
  }
  if ( fields & LWT_COL_EDGE_FACE_LEFT ) {
    appendStringInfo(str, "%sleft_face", sep);
    appendStringInfo(str, "%s %lld", op, edge->face_left);
    sep = sep1;
  }
  if ( fields & LWT_COL_EDGE_FACE_RIGHT ) {
    appendStringInfo(str, "%sright_face", sep);
    appendStringInfo(str, "%s %lld", op, edge->face_right);
    sep = sep1;
  }
  if ( fields & LWT_COL_EDGE_NEXT_LEFT ) {
    appendStringInfo(str, "%snext_left_edge", sep);
    appendStringInfo(str, "%s %lld", op, edge->next_left);
    sep = sep1;
    if ( fullEdgeData ) {
      appendStringInfo(str, "%s abs_next_left_edge", sep);
      appendStringInfo(str, "%s %lld", op, llabs(edge->next_left));
    }
  }
  if ( fields & LWT_COL_EDGE_NEXT_RIGHT ) {
    appendStringInfo(str, "%snext_right_edge", sep);
    appendStringInfo(str, "%s %lld", op, edge->next_right);
    sep = sep1;
    if ( fullEdgeData ) {
      appendStringInfo(str, "%s abs_next_right_edge", sep);
      appendStringInfo(str, "%s %lld", op, llabs(edge->next_right));
    }
  }
  if ( fields & LWT_COL_EDGE_GEOM ) {
    appendStringInfo(str, "%sgeom", sep);
    hexewkb = lwgeom_to_hexwkb(lwline_as_lwgeom(edge->geom),
                                WKB_EXTENDED, &hexewkb_size);
    appendStringInfo(str, "%s'%s'::geometry", op, hexewkb);
    lwfree(hexewkb);
  }
}

static void
addNodeFields(StringInfo str, int fields)
{
  const char *sep = "";

  if ( fields & LWT_COL_NODE_NODE_ID ) {
    appendStringInfoString(str, "node_id");
    sep = ",";
  }
  if ( fields & LWT_COL_NODE_CONTAINING_FACE ) {
    appendStringInfo(str, "%scontaining_face", sep);
    sep = ",";
  }
  if ( fields & LWT_COL_NODE_GEOM ) {
    appendStringInfo(str, "%sgeom", sep);
  }
}

/* Add node values for an insert, in text form */
static void
addNodeValues(StringInfo str, LWT_ISO_NODE *node)
{
  size_t hexewkb_size;
  char *hexewkb;

  if ( node->node_id != -1 ) appendStringInfo(str, "(%lld", node->node_id);
  else appendStringInfoString(str, "(DEFAULT");

  if ( node->containing_face != -1 )
    appendStringInfo(str, ",%lld", node->containing_face);
  else appendStringInfoString(str, ",null");

  if ( node->geom ) {
    hexewkb = lwgeom_to_hexwkb(lwpoint_as_lwgeom(node->geom),
                                WKB_EXTENDED, &hexewkb_size);
    appendStringInfo(str, ",'%s'::geometry)", hexewkb);
    lwfree(hexewkb);
  } else {
    appendStringInfoString(str, ",null)");
  }
}

static void
fillEdgeFields(LWT_ISO_EDGE* edge, HeapTuple row, TupleDesc rowdesc, int fields)
{
  bool isnull;
  Datum dat;
  GSERIALIZED *geom;
  int colno = 0;

  if ( fields & LWT_COL_EDGE_EDGE_ID ) {
    dat = SPI_getbinval(row, rowdesc, ++colno, &isnull);
    edge->edge_id = DatumGetInt32(dat);
  }
  if ( fields & LWT_COL_EDGE_START_NODE ) {
    dat = SPI_getbinval(row, rowdesc, ++colno, &isnull);
    edge->start_node = DatumGetInt32(dat);
  }
  if ( fields & LWT_COL_EDGE_END_NODE ) {
    dat = SPI_getbinval(row, rowdesc, ++colno, &isnull);
    edge->end_node = DatumGetInt32(dat);
  }
  if ( fields & LWT_COL_EDGE_FACE_LEFT ) {
    dat = SPI_getbinval(row, rowdesc, ++colno, &isnull);
    edge->face_left = DatumGetInt32(dat);
  }
  if ( fields & LWT_COL_EDGE_FACE_RIGHT ) {
    dat = SPI_getbinval(row, rowdesc, ++colno, &isnull);
    edge->face_right = DatumGetInt32(dat);
  }
  if ( fields & LWT_COL_EDGE_NEXT_LEFT ) {
    dat = SPI_getbinval(row, rowdesc, ++colno, &isnull);
    edge->next_left = DatumGetInt32(dat);
  }
  if ( fields & LWT_COL_EDGE_NEXT_RIGHT ) {
    dat = SPI_getbinval(row, rowdesc, ++colno, &isnull);
    edge->next_right = DatumGetInt32(dat);
  }
  if ( fields & LWT_COL_EDGE_GEOM ) {
    dat = SPI_getbinval(row, rowdesc, ++colno, &isnull);
    if ( ! isnull ) {
      geom = (GSERIALIZED *)PG_DETOAST_DATUM_COPY(dat);
      edge->geom = lwgeom_as_lwline(lwgeom_from_gserialized(geom));
    } else {
      lwpgnotice("Found edge with NULL geometry !");
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
  int colno = 0;

  if ( fields & LWT_COL_NODE_NODE_ID ) {
    dat = SPI_getbinval(row, rowdesc, ++colno, &isnull);
    node->node_id = DatumGetInt32(dat);
  }
  if ( fields & LWT_COL_NODE_CONTAINING_FACE ) {
    dat = SPI_getbinval(row, rowdesc, ++colno, &isnull);
    if ( isnull ) node->containing_face = -1;
    else node->containing_face = DatumGetInt32(dat);
  }
  if ( fields & LWT_COL_EDGE_GEOM ) {
    dat = SPI_getbinval(row, rowdesc, ++colno, &isnull);
    if ( ! isnull ) {
      geom = (GSERIALIZED *)PG_DETOAST_DATUM_COPY(dat);
      node->geom = lwgeom_as_lwpoint(lwgeom_from_gserialized(geom));
    } else {
      lwpgnotice("Found node with NULL geometry !");
      node->geom = NULL;
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

static LWT_ISO_EDGE*
cb_getEdgeById(const LWT_BE_TOPOLOGY* topo,
      const LWT_ELEMID* ids, int* numelems, int fields)
{
  LWT_ISO_EDGE *edges;
	int spi_result;

  StringInfoData sqldata;
  StringInfo sql = &sqldata;
  int i;

  initStringInfo(sql);
  appendStringInfoString(sql, "SELECT ");
  addEdgeFields(sql, fields, 0);
  appendStringInfo(sql, " FROM \"%s\".edge_data", topo->name);
  appendStringInfoString(sql, " WHERE edge_id IN (");
  // add all identifiers here
  for (i=0; i<*numelems; ++i) {
    appendStringInfo(sql, "%s%lld", (i?",":""), ids[i]);
  }
  appendStringInfoString(sql, ")");
  spi_result = SPI_execute(sql->data, true, *numelems);
  if ( spi_result != SPI_OK_SELECT ) {
		cberror(topo->be_data, "unexpected return (%d) from query execution: %s", spi_result, sql->data);
	  *numelems = -1; return NULL;
  }
  pfree(sqldata.data);

  lwpgnotice("cb_getEdgeById: edge query returned %d rows", SPI_processed);
  *numelems = SPI_processed;
  if ( ! SPI_processed ) {
    return NULL;
  }

  edges = palloc( sizeof(LWT_ISO_EDGE) * SPI_processed );
  for ( i=0; i<SPI_processed; ++i )
  {
    HeapTuple row = SPI_tuptable->vals[i];
    fillEdgeFields(&edges[i], row, SPI_tuptable->tupdesc, fields);
  }

  return edges;
}

static LWT_ISO_NODE*
cb_getNodeWithinDistance2D(const LWT_BE_TOPOLOGY* topo,
      const LWPOINT* pt, double dist, int* numelems,
      int fields, int limit)
{
  LWT_ISO_NODE *nodes;
	int spi_result;
  size_t hexewkb_size;
  char *hexewkb;
  StringInfoData sqldata;
  StringInfo sql = &sqldata;
  int elems_requested = limit;
  int i;

  initStringInfo(sql);
  if ( elems_requested == -1 ) {
    appendStringInfoString(sql, "SELECT EXISTS ( SELECT 1");
  } else {
    appendStringInfoString(sql, "SELECT ");
    if ( fields ) addNodeFields(sql, fields);
    else {
      lwpgwarning("liblwgeom-topo invoked 'getNodeWithinDistance2D' "
                  "backend callback with limit=%d and no fields",
                  elems_requested);
      appendStringInfo(sql, "*");
    }
  }
  appendStringInfo(sql, " FROM \"%s\".node", topo->name);
  // TODO: use binary cursor here ?
  hexewkb = lwgeom_to_hexwkb(lwpoint_as_lwgeom(pt), WKB_EXTENDED, &hexewkb_size);
  if ( dist ) {
    appendStringInfo(sql, " WHERE ST_DWithin(geom, '%s'::geometry, dist)",
                     hexewkb);
  } else {
    appendStringInfo(sql, " WHERE ST_Within(geom, '%s'::geometry)", hexewkb);
  }
  lwfree(hexewkb);
  if ( elems_requested == -1 ) {
    appendStringInfoString(sql, ")");
  } else if ( elems_requested > 0 ) {
    appendStringInfo(sql, " LIMIT %d", elems_requested);
  }
  spi_result = SPI_execute(sql->data, true, *numelems);
  if ( spi_result != SPI_OK_SELECT ) {
		cberror(topo->be_data, "unexpected return (%d) from query execution: %s",
            spi_result, sql->data);
	  *numelems = -1; return NULL;
  }
  pfree(sqldata.data);

  lwpgnotice("cb_getNodeWithinDistance2D: edge query "
             "(limited by %d) returned %d rows",
             elems_requested, SPI_processed);
  if ( ! SPI_processed ) {
    *numelems = 0; return NULL;
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
    return NULL;
  }
  else
  {
    nodes = palloc( sizeof(LWT_ISO_EDGE) * SPI_processed );
    for ( i=0; i<SPI_processed; ++i )
    {
      HeapTuple row = SPI_tuptable->vals[i];
      fillNodeFields(&nodes[i], row, SPI_tuptable->tupdesc, fields);
    }
    *numelems = SPI_processed;
    return nodes;
  }
}

static int
cb_insertNodes( const LWT_BE_TOPOLOGY* topo,
      LWT_ISO_NODE* nodes, int numelems )
{
	int spi_result;
  StringInfoData sqldata;
  StringInfo sql = &sqldata;
  int i;

  initStringInfo(sql);
  appendStringInfo(sql, "INSERT INTO \"%s\".node (", topo->name);
  addNodeFields(sql, LWT_COL_NODE_ALL);
  appendStringInfoString(sql, ") VALUES ");
  for ( i=0; i<numelems; ++i ) {
    if ( i ) appendStringInfoString(sql, ",");
    // TODO: prepare and execute ?
    addNodeValues(sql, &nodes[i]);
  }
  appendStringInfoString(sql, " RETURNING node_id");

  spi_result = SPI_execute(sql->data, false, numelems);
  if ( spi_result != SPI_OK_INSERT_RETURNING ) {
		cberror(topo->be_data, "unexpected return (%d) from query execution: %s",
            spi_result, sql->data);
	  return 0;
  }
  pfree(sqldata.data);

  if ( SPI_processed != numelems ) {
		cberror(topo->be_data, "processed %d rows, expected %d",
            SPI_processed, numelems);
	  return 0;
  }

  /* Set node_id (could skip this if none had it set to -1) */
  /* TODO: check for -1 values in the first loop */
  for ( i=0; i<SPI_processed; ++i )
  {
    if ( nodes[i].node_id != -1 ) continue;
    fillNodeFields(&nodes[i], SPI_tuptable->vals[i],
      SPI_tuptable->tupdesc, LWT_COL_NODE_NODE_ID);
  }

  return 1;
}

static int
cb_insertEdges( const LWT_BE_TOPOLOGY* topo,
      LWT_ISO_EDGE* edges, int numelems )
{
	int spi_result;
  StringInfoData sqldata;
  StringInfo sql = &sqldata;
  int i;
  int needsEdgeIdReturn = 0;

  initStringInfo(sql);
  /* NOTE: we insert into "edge", on which an insert rule is defined */
  appendStringInfo(sql, "INSERT INTO \"%s\".edge_data (", topo->name);
  addEdgeFields(sql, LWT_COL_EDGE_ALL, 1);
  appendStringInfoString(sql, ") VALUES ");
  for ( i=0; i<numelems; ++i ) {
    if ( i ) appendStringInfoString(sql, ",");
    // TODO: prepare and execute ?
    addEdgeValues(sql, &edges[i], 1);
    if ( edges[i].edge_id == -1 ) needsEdgeIdReturn = 1;
  }
  if ( needsEdgeIdReturn ) appendStringInfoString(sql, " RETURNING edge_id");

  spi_result = SPI_execute(sql->data, false, numelems);
  if ( spi_result != ( needsEdgeIdReturn ? SPI_OK_INSERT_RETURNING : SPI_OK_INSERT ) )
  {
		cberror(topo->be_data, "unexpected return (%d) from query execution: %s",
            spi_result, sql->data);
	  return -1;
  }
  pfree(sqldata.data);

  if ( SPI_processed != numelems ) {
		cberror(topo->be_data, "processed %d rows, expected %d",
            SPI_processed, numelems);
	  return -1;
  }

  if ( needsEdgeIdReturn )
  {
    /* Set node_id for items that need it */
    for ( i=0; i<SPI_processed; ++i )
    {
      if ( edges[i].edge_id != -1 ) continue;
      fillEdgeFields(&edges[i], SPI_tuptable->vals[i],
        SPI_tuptable->tupdesc, LWT_COL_EDGE_EDGE_ID);
    }
  }

  return SPI_processed;
}

static int
cb_updateEdges( const LWT_BE_TOPOLOGY* topo,
      const LWT_ISO_EDGE* sel_edge, int sel_fields,
      const LWT_ISO_EDGE* upd_edge, int upd_fields,
      const LWT_ISO_EDGE* exc_edge, int exc_fields )
{
	int spi_result;
  StringInfoData sqldata;
  StringInfo sql = &sqldata;

  initStringInfo(sql);
  appendStringInfo(sql, "UPDATE \"%s\".edge_data SET ", topo->name);
  addEdgeUpdate( sql, upd_edge, upd_fields, 1, updSet );
  if ( exc_edge || sel_edge ) appendStringInfoString(sql, " WHERE ");
  if ( sel_edge ) {
    addEdgeUpdate( sql, sel_edge, sel_fields, 1, updSel );
    if ( exc_edge ) appendStringInfoString(sql, " AND ");
  }
  if ( exc_edge ) {
    addEdgeUpdate( sql, exc_edge, exc_fields, 1, updNot );
  }

  /* lwpgnotice("cb_updateEdges: %s", sql->data); */

  spi_result = SPI_execute( sql->data, false, 0 );
  if ( spi_result != SPI_OK_UPDATE )
  {
		cberror(topo->be_data, "unexpected return (%d) from query execution: %s",
            spi_result, sql->data);
	  return -1;
  }
  pfree(sqldata.data);

  lwpgnotice("cb_updateEdges: update query processed %d rows", SPI_processed);

  return SPI_processed;
}

static LWT_ELEMID
cb_getNextEdgeId( const LWT_BE_TOPOLOGY* topo )
{
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
  if ( spi_result != SPI_OK_SELECT ) {
		cberror(topo->be_data, "unexpected return (%d) from query execution: %s",
            spi_result, sql->data);
	  return -1;
  }
  pfree(sqldata.data);

  if ( SPI_processed != 1 ) {
		cberror(topo->be_data, "processed %d rows, expected 1", SPI_processed);
	  return -1;
  }

  dat = SPI_getbinval( SPI_tuptable->vals[0],
                       SPI_tuptable->tupdesc, 1, &isnull );
  if ( isnull ) {
		cberror(topo->be_data, "nextval for edge_id returned null");
	  return -1;
  }
  edge_id = DatumGetInt32(dat);
  return edge_id;
}

static int
cb_updateTopoGeomEdgeSplit ( const LWT_BE_TOPOLOGY* topo,
  LWT_ELEMID split_edge, LWT_ELEMID new_edge1, LWT_ELEMID new_edge2 )
{
	int spi_result;
  StringInfoData sqldata;
  StringInfo sql = &sqldata;
  int i, ntopogeoms;
  const char *proj = "r.element_id, r.topogeo_id, r.layer_id, r.element_type";

  initStringInfo(sql);
  if ( new_edge2 == -1 ) {
    appendStringInfo(sql, "SELECT %s", proj);
  } else {
    appendStringInfoString(sql, "DELETE");
  }
  appendStringInfo(sql, " FROM \"%s\".relation r, topology.layer l WHERE "
    "l.topology_id = %d AND l.level = 0 AND l.layer_id = r.layer_id "
    "AND abs(r.element_id) = %lld AND r.element_type = 2",
    topo->name, topo->id, split_edge);
  if ( new_edge2 != -1 ) {
    appendStringInfo(sql, " RETURNING %s", proj);
  }

  spi_result = SPI_execute(sql->data, true, 0);
  if ( spi_result != ( new_edge2 == -1 ? SPI_OK_SELECT : SPI_OK_DELETE_RETURNING ) ) {
		cberror(topo->be_data, "unexpected return (%d) from query execution: %s",
            spi_result, sql->data);
	  return 0;
  }
  ntopogeoms = SPI_processed;
  for ( i=0; i<ntopogeoms; ++i )
  {
    HeapTuple row = SPI_tuptable->vals[i];
    TupleDesc tdesc = SPI_tuptable->tupdesc;
    int negate;
    int element_id;
    int topogeo_id;
    int layer_id;
    int element_type;

    if ( ! getNotNullInt32( row, tdesc, 1, &element_id ) ) {
		  cberror(topo->be_data,
        "unexpected null element_id in \"%s\".relation",
        topo->name);
	    return 0;
    }
    negate = ( element_id < 0 );

    if ( ! getNotNullInt32( row, tdesc, 2, &topogeo_id ) ) {
		  cberror(topo->be_data,
        "unexpected null topogeo_id in \"%s\".relation",
        topo->name);
	    return 0;
    }

    if ( ! getNotNullInt32( row, tdesc, 3, &layer_id ) ) {
		  cberror(topo->be_data,
        "unexpected null layer_id in \"%s\".relation",
        topo->name);
	    return 0;
    }

    if ( ! getNotNullInt32( row, tdesc, 4, &element_type ) ) {
		  cberror(topo->be_data,
        "unexpected null element_type in \"%s\".relation",
        topo->name);
	    return 0;
    }

    resetStringInfo(sql);
    appendStringInfo(sql,
      "INSERT INTO \"%s\".relation VALUES ("
      "%d,%d,%lld,%d)", topo->name,
      topogeo_id, layer_id, negate ? -new_edge1 : new_edge1, element_type);
    spi_result = SPI_execute(sql->data, false, 0);
    if ( spi_result != SPI_OK_INSERT ) {
      cberror(topo->be_data, "unexpected return (%d) from query execution: %s",
              spi_result, sql->data);
      return 0;
    }
    if ( new_edge2 != -1 ) {
      resetStringInfo(sql);
      appendStringInfo(sql,
        "INSERT INTO FROM \"%s\".relation VALUES ("
        "%d,%d,%lld,%d", topo->name,
        topogeo_id, layer_id, negate ? -new_edge2 : new_edge2, element_type);
      spi_result = SPI_execute(sql->data, false, 0);
      if ( spi_result != SPI_OK_INSERT ) {
        cberror(topo->be_data, "unexpected return (%d) from query execution: %s",
                spi_result, sql->data);
        return 0;
      }
    }
  }

  lwpgnotice("cb_updateTopoGeomEdgeSplit: updated %d topogeoms", ntopogeoms);

  return 1;
}

LWT_BE_CALLBACKS be_callbacks = {
    cb_lastErrorMessage,
    NULL, /* createTopology */
    cb_loadTopologyByName, /* loadTopologyByName */
    cb_freeTopology, /* freeTopology */
    NULL, /* getNodeById */
    cb_getNodeWithinDistance2D,
    cb_insertNodes,
    cb_getEdgeById, /* getEdgeById */
    NULL, /* getEdgeWithinDistance2D */
    cb_getNextEdgeId,
    cb_insertEdges,
    cb_updateEdges,
    NULL, /* getFacesById */
    NULL, /* getFaceContainingPoint */
    cb_updateTopoGeomEdgeSplit
};


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

  if ( PG_ARGISNULL(0) || PG_ARGISNULL(1) || PG_ARGISNULL(2) ) {
    lwpgerror("SQL/MM Spatial exception - null argument");
    PG_RETURN_NULL();
  }

  toponame_text = PG_GETARG_TEXT_P(0);
  toponame = text2cstring(toponame_text);
	PG_FREE_IF_COPY(toponame_text, 0);

  edge_id = PG_GETARG_INT32(1) ;

  geom = PG_GETARG_GSERIALIZED_P(2);
  lwgeom = lwgeom_from_gserialized(geom);
  pt = lwgeom_as_lwpoint(lwgeom);
  if ( ! pt ) {
    lwgeom_free(lwgeom);
	  PG_FREE_IF_COPY(geom, 3);
    lwpgerror("ST_ModEdgeSplit third argument must be a point geometry");
    PG_RETURN_NULL();
  }

  if ( SPI_OK_CONNECT != SPI_connect() ) {
    lwpgerror("Could not connect to SPI");
    PG_RETURN_NULL();
  }

  topo = lwt_LoadTopology(be_iface, toponame);
  pfree(toponame);
  if ( ! topo ) {
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

  if ( node_id == -1 ) {
    /* should never reach this point, as lwerror would raise an exception */
    SPI_finish();
    PG_RETURN_NULL();
  }

  SPI_finish();
  PG_RETURN_INT32(node_id);
}
