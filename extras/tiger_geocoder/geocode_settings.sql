--
-- PostGIS - Spatial Types for PostgreSQL
-- http://postgis.net
--
-- Copyright (C) 2010, 2011-2015 Regina Obe and Leo Hsu
--
-- This is free software; you can redistribute and/or modify it under
-- the terms of the GNU General Public Licence. See the COPYING file.
--
-- Author: Regina Obe and Leo Hsu <lr@pcorp.us>
--
-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
--
SELECT tiger.SetSearchPathForInstall('tiger');

CREATE OR REPLACE FUNCTION install_geocode_settings()
	RETURNS void AS
$$
DECLARE var_temp text;
BEGIN
	var_temp := tiger.SetSearchPathForInstall('tiger'); /** set set search path to have tiger in front **/
	IF NOT EXISTS(SELECT table_name FROM information_schema.columns WHERE table_schema = 'tiger' AND table_name = 'geocode_settings')  THEN
		CREATE TABLE geocode_settings(name text primary key, setting text, unit text, category text, short_desc text);
		GRANT SELECT ON geocode_settings TO public;
	END IF;
	IF NOT EXISTS(SELECT table_name FROM information_schema.columns WHERE table_schema = 'tiger' AND table_name = 'geocode_settings_default')  THEN
		CREATE TABLE geocode_settings_default(name text primary key, setting text, unit text, category text, short_desc text);
		GRANT SELECT ON geocode_settings_default TO public;
	END IF;
	--recreate defaults
	TRUNCATE TABLE geocode_settings_default;
	INSERT INTO geocode_settings_default(name,setting,unit,category,short_desc)
		SELECT f.*
		FROM
		(VALUES ('debug_geocode_address', 'false', 'boolean','debug', 'outputs debug information in notice log such as queries when geocode_addresss is called if true')
			, ('debug_geocode_intersection', 'false', 'boolean','debug', 'outputs debug information in notice log such as queries when geocode_intersection is called if true')
			, ('debug_normalize_address', 'false', 'boolean','debug', 'outputs debug information in notice log such as queries and intermediate expressions when normalize_address is called if true')
			, ('debug_reverse_geocode', 'false', 'boolean','debug', 'if true, outputs debug information in notice log such as queries and intermediate expressions when reverse_geocode')
			, ('reverse_geocode_numbered_roads', '0', 'integer','rating', 'For state and county highways, 0 - no preference in name, 1 - prefer the numbered highway name, 2 - prefer local state/county name')
			, ('use_pagc_address_parser', 'false', 'boolean','normalize', 'If set to true, will try to use the address_standardizer extension (via pagc_normalize_address) instead of tiger normalize_address built on')
			, ('zip_penalty', '2', 'numeric','rating', 'As input to rating will add (ref_zip - tar_zip)*zip_penalty where ref_zip is input address and tar_zip is a target address candidate')
		) f(name,setting,unit,category,short_desc);

	-- delete entries that are the same as default values
	DELETE FROM geocode_settings As gc USING geocode_settings_default As gf WHERE gf.name = gc.name AND gf.setting = gc.setting;
END;
$$
language plpgsql;

SELECT install_geocode_settings(); /** create the table if it doesn't exist **/

CREATE OR REPLACE FUNCTION get_geocode_setting(setting_name text)
RETURNS text AS
$$
SELECT COALESCE(gc.setting,gd.setting) As setting FROM geocode_settings_default AS gd LEFT JOIN geocode_settings AS gc ON gd.name = gc.name  WHERE gd.name = $1;
$$
language sql STABLE;

CREATE OR REPLACE FUNCTION set_geocode_setting(setting_name text, setting_value text)
RETURNS text AS
$$
INSERT INTO geocode_settings(name, setting, unit, category, short_desc)
SELECT name, setting, unit, category, short_desc
    FROM geocode_settings_default
    WHERE name NOT IN(SELECT name FROM geocode_settings);

UPDATE geocode_settings SET setting = $2 WHERE name = $1
	RETURNING setting;
$$
language sql VOLATILE;
