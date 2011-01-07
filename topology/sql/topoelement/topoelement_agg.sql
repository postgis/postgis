-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
-- 
-- PostGIS - Spatial Types for PostgreSQL
-- http://postgis.refractions.net
--
-- Copyright (C) 2010 Sandro Santilli <strk@keybit.net>
--
-- This is free software; you can redistribute and/or modify it under
-- the terms of the GNU General Public Licence. See the COPYING file.
--
-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
--
-- TopoElement management functions 
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
-- TopoElementArray TopoElementArray_append(<TopoElement>)
--
-- Append a TopoElement to a TopoElementArray
--
CREATE OR REPLACE FUNCTION topology.TopoElementArray_append(
		topology.TopoElementArray, topology.TopoElement)
	RETURNS topology.TopoElementArray
AS
$$
	SELECT CASE
		WHEN $1 IS NULL THEN
			topology.TopoElementArray('{' || $2::text || '}')
		ELSE
			topology.TopoElementArray($1::int[][]||$2::int[])
		END;
$$
LANGUAGE 'sql';
--} TopoElementArray_append

--{
--
-- TopoElementArray TopoElementArray_agg(<setof TopoElement>)
--
-- Aggregates a set of TopoElement values into a TopoElementArray
--
DROP AGGREGATE IF EXISTS topology.TopoElementArray_agg(topology.TopoElement);
CREATE AGGREGATE topology.TopoElementArray_agg(
	sfunc = topology.TopoElementArray_append,
	basetype = topology.TopoElement,
	stype = topology.TopoElementArray
	);
--} TopoElementArray_agg
