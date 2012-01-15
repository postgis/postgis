-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
-- 
-- PostGIS - Spatial Types for PostgreSQL
-- http://postgis.refractions.net
--
-- Copyright (C) 2011 Sandro Santilli <strk@keybit.net>
--
-- This is free software; you can redistribute and/or modify it under
-- the terms of the GNU General Public Licence. See the COPYING file.
--
-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
--
-- Developed by Sandro Santilli <strk@keybit.net>
-- for Faunalia (http://www.faunalia.it) with funding from
-- Regione Toscana - Sistema Informativo per la Gestione del Territorio
-- e dell' Ambiente [RT-SIGTA].
-- For the project: "Sviluppo strumenti software per il trattamento di dati
-- geografici basati su QuantumGIS e Postgis (CIG 0494241492)"
--
-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

--{
--
-- Return a list of edges (sequence, id) resulting by starting from the
-- given edge and following the leftmost turn at each encountered node.
--
-- Edge ids are signed, they are negative if traversed backward. 
-- Sequence numbers start with 1.
--
-- Use a negative starting_edge to follow its rigth face rather than
-- left face (to start traversing it in reverse).
--
-- Optionally pass a limit on the number of edges to traverse. This is a
-- safety measure against not-properly linked topologies, where you may
-- end up looping forever (single edge loops edge are detected but longer
-- ones are not). Default is no limit (good luck!)
--
-- GetRingEdges(atopology, anedge, [maxedges])
--
CREATE OR REPLACE FUNCTION topology.GetRingEdges(atopology varchar, anedge int, maxedges int DEFAULT null)
	RETURNS SETOF topology.GetFaceEdges_ReturnType
AS
$$
DECLARE
  curedge int;
  nextedge int;
  rec RECORD;
  bounds geometry;
  retrec topology.GetFaceEdges_ReturnType;
  n int;
  sql text;
BEGIN

  curedge := anedge;
  n := 1;

  WHILE true LOOP
    sql := 'SELECT edge_id, next_left_edge, next_right_edge FROM '
      || quote_ident(atopology) || '.edge_data WHERE edge_id = '
      || abs(curedge);
    EXECUTE sql INTO rec;
    retrec.sequence := n;
    retrec.edge := curedge;

    RAISE DEBUG 'Edge:% left:% right:%',
      curedge, rec.next_left_edge, rec.next_right_edge;

    RETURN NEXT retrec;

    IF curedge < 0 THEN
      nextedge := rec.next_right_edge;
    ELSE
      nextedge := rec.next_left_edge;
    END IF;

    IF nextedge = anedge THEN
      RAISE DEBUG ' finish';
      RETURN;
    END IF;

    IF nextedge = curedge THEN
      RAISE EXCEPTION 'Detected bogus loop traversing edge %', curedge;
    END IF;

    curedge := nextedge;

    RAISE DEBUG ' curedge:% anedge:%', curedge, anedge;

    n := n + 1;

    IF n > maxedges THEN
      RAISE EXCEPTION 'Max traversing limit hit: %', maxedges;
    END IF;
  END LOOP;

END
$$
LANGUAGE 'plpgsql' STABLE;
--} GetRingEdges
