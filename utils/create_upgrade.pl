#!/usr/bin/env perl

#
# PostGIS - Spatial Types for PostgreSQL
# http://postgis.net
#
# Copyright (C) 2014-2021 Sandro Santilli <strk@kbt.io>
# Copyright (C) 2009-2010 Paul Ramsey <pramsey@opengeo.org>
# Copyright (C) 2005 Refractions Research Inc.
#
# This is free software; you can redistribute and/or modify it under
# the terms of the GNU General Public Licence. See the COPYING file.
#

#
# This script produces an .sql file containing
# CREATE OR REPLACE calls for each function
# in postgis.sql
#
# In addition, the transaction contains
# a check for Major postgis_lib_version()
# to match the one contained in lwpostgis.sql
#
# This never happens by just running make install
# as MODULE_FILENAME contains SO_MAJOR under
# all architectures.
#
#

eval "exec perl -w $0 $@"
  if (0);

use strict;
use warnings;

#
# Conditionally upgraded types and operators based
# on their last updated version and the version of
# the target database
#

sub parse_last_updated
{
    my $comment = shift;
    if ( $comment =~ m/.*(?:Availability|Changed|Updated):\s([^\.])\.([^.]*)/s )
    {
        return $1*100 + $2;
    }
    return 0;
}

sub parse_replaces
{
    my @replaces = ();
    my $comment = shift;
    my ($name, $args, $ver);
    foreach my $line ( split /\n/, $comment )
    {
        if ( $line =~ m/.*Replaces\s\s*([^\(]*)\(([^\)]*)\)\s\s*deprecated in\s\s*([^\.]*)\.([^.]*)/ )
        {
            $name = $1;
            $args = $2;
            $ver = $3*100 + $4;
            my @r = ($name, $args, $ver);
            push @replaces, \@r;
        }
    }
    return @replaces;
}

sub parse_missing
{
    my $comment = shift;
    my @missing = ();
    if ( $comment =~ m/.*(?:Missing in):\s([^\.])\.([^.]*)/s )
    {
        push(@missing, $1*100 + $2);
    }
    return join(',',@missing);
}

#
# Commandline argument handling
#
($#ARGV == 0)
  ||die
"Usage: perl create_upgrade.pl <create_script.sql> [<schema>]\nCreates a new SQL script to upgrade all of the PostGIS functions.\n"
  if ( @ARGV < 1 || @ARGV > 3 );

my $sql_file = $ARGV[0];
my $module = 'postgis';
my $soname = '';
my $version_to = "";
my $version_to_full = "";
my $version_to_num = 0;
my $schema = "";
$schema = $ARGV[1] if @ARGV > 1;

my @renamed_deprecated_functions = ();

die "Unable to open input SQL file $sql_file\n"
  if ( !-f $sql_file );

#
# Search the SQL file for the target version number (the
# version we are upgrading *to*.
#
open( INPUT, $sql_file ) || die "Couldn't open file: $sql_file\n";
while(<INPUT>)
{
    #
    # Since 1.1.0 scripts/lib/release versions are the same
    #
    if (/INSTALL VERSION: (.*)/)
    {
        $version_to = $1;

        #last;
    }
    elsif (/TYPE raster/)
    {
        $module = 'postgis_raster';
    }
    elsif (m@('\$libdir/[^']*')@)
    {
        $soname = $1;
    }
}
close(INPUT);

die "Unable to locate target new version number in $sql_file\n"
  if( !$version_to );

if ( $version_to =~ /(\d+)\.(\d+)\.(\d+)([^' ]*)/ )
{
    $version_to_full = $1 . '.' . $2 . '.' . $3 . $4;
    $version_to = $1 . "." . $2;
    $version_to_num = 100 * $1 + $2;
}
else
{
    die "Version to number invalid, must be of form X.X.X\n";
}

print qq{
--
-- UPGRADE SCRIPT TO PostGIS $version_to_full
--

};

print "LOAD $soname;\n" if ($soname);

#print "BEGIN;\n";
print "SET search_path TO $schema;\n" if $schema;

#
# Add in the conditional check function to ensure this script is
# not being applied to a major version update.
#
while(<DATA>)
{
    s/NEWVERSION/$version_to_full/g;
    s/MODULE/$module/g;
    print;
}

#
# Go through the SQL file and strip out objects that cannot be
# applied to an existing, loaded database: types and operators
# and operator classes that have already been defined.
#
my $comment = '';
open( INPUT, $sql_file ) || die "Couldn't open file: $sql_file\n";
while(<INPUT>)
{

    if (/^\-\-/)
    {
        $comment .= $_;
        next;
    }

    #
    # Allow through deprecations from postgis_drop.sql
    #
    print if (/^drop function /i);
    print if (/^drop aggregate /i);

    if (/^create or replace function/i)
    {
        my $def .= $_;
        my @replaced_array = parse_replaces($comment);
        my $endfunc = 0;
        while(<INPUT>)
        {
            $def .= $_;
            $endfunc = 1 if /^\s*(\$\$\s*)?LANGUAGE /;
            last if ( $endfunc && /\;/ );
        }
        foreach my $replaced (@replaced_array)
        {
            my ($name, $args, $ver) = @$replaced;
            $name = lc($name); # lowercase the name
            my $renamed = $name . '_deprecated_by_postgis_' . ${ver};
            my $replacement = "${renamed}(${args})";
            push @renamed_deprecated_functions, ${renamed};
            print <<"EOF";
-- Rename $name ( $args ) deprecated in PostGIS $ver
DO LANGUAGE 'plpgsql'
\$postgis_proc_upgrade\$
DECLARE
    detail TEXT;
BEGIN
    -- Rename the replaced function, to avoid ambiguities.
    -- The renamed function will eventually be drop.
    BEGIN
        ALTER FUNCTION $name( $args ) RENAME TO ${renamed};
    EXCEPTION
    WHEN undefined_function THEN
        RAISE DEBUG 'Replaced function $name($args) does not exist';
    WHEN OTHERS THEN
        GET STACKED DIAGNOSTICS detail := PG_EXCEPTION_DETAIL;
        RAISE EXCEPTION 'Attempting to rename replaced function $name($args) got % (%)', SQLERRM, SQLSTATE
            USING DETAIL = detail;
    END;

END;
\$postgis_proc_upgrade\$;
EOF
        }

        # TODO: set owner to current owner, in case the new signature exists

        print $def;
    }

    if (/^create type (\w+)/i)
    {
        my $newtype = $1;
        my $def .= $_;
        while(<INPUT>)
        {
            $def .= $_;
            last if /\)/;
        }

        my $last_updated = parse_last_updated($comment);
        if ( !$last_updated )
        {
            die "ERROR: no last updated info for type '${newtype}'\n";
        }
        my $missing = parse_missing($comment);
        print "-- Type ${newtype} -- LastUpdated: ${last_updated}\n";
        print <<"EOF";
DO LANGUAGE 'plpgsql'
\$postgis_proc_upgrade\$
BEGIN
  IF $last_updated > version_from_num
EOF
        print "OR version_from_num IN ( ${missing} )" if ($missing);
        print <<"EOF";
     FROM _postgis_upgrade_info()
  THEN
      EXECUTE \$postgis_proc_upgrade_parsed_def\$ $def \$postgis_proc_upgrade_parsed_def\$;
  END IF;
END
\$postgis_proc_upgrade\$;
EOF
    }

    if (/^do *language .*\$\$/i)
    {
        print;
        while(<INPUT>)
        {
            print;
            last if /\$\$/;
        }
    }

    # This code handles casts by dropping and recreating them.
    if (/^create cast\s+\(\s*(\w+)\s+as\s+(\w+)\)/i)
    {
        my $type1 = $1;
        my $type2 = $2;
        my $def = $_;
        unless (/;$/)
        {
            while(<INPUT>)
            {
                $def .= $_;
                last if /;$/;
            }
        }
        print "DROP CAST IF EXISTS ($type1 AS $type2);\n";
        print $def;
    }

    # This code handles aggregates by dropping and recreating them.
    # For PG12 use REPLACE instead
    if (/^create aggregate\s+([^(]+)\s*\(/i)
    {
        my $aggname = $1;

        #print "-- Aggname ${aggname}\n";
        my $aggtype = 'unknown';
        my $def = $_;
        if (/^create aggregate\s+\S+\s*\(([^)]*)\)/i)
        {
            $aggtype = $1;
            $aggtype =~ s/\s*,\s*/,/g; # drop spaces around commas
            $aggtype =~ s/\s\s*/ /g; # collapse multiple spaces into one
        }
        while(<INPUT>)
        {
            $def .= $_;
            $aggtype = $1 if (/basetype\s*=\s*([^,]*)\s*,/i);
            last if /\);/;
        }
        my $aggsig = "$aggname($aggtype)";

        #print "-- Checking comment $comment\n";
        my $last_updated = parse_last_updated($comment);
        if ( !$last_updated )
        {
            die "ERROR: no last updated info for aggregate '${aggsig}'\n";
        }

        my $pg12_def = $def;
        $pg12_def =~ s/CREATE AGGREGATE/CREATE OR REPLACE AGGREGATE/;
        if ($pg12_def eq "")
        {
            $pg12_def = "RAISE EXCEPTION 'Could not parse AGGREGATE'";
        }
        print "-- Aggregate ${aggsig} -- LastUpdated: ${last_updated}\n";
        print <<"EOF";
DO LANGUAGE 'plpgsql'
\$postgis_proc_upgrade\$
BEGIN
  IF pg_catalog.current_setting('server_version_num')::integer >= 120000
  THEN
    EXECUTE \$postgis_proc_upgrade_parsed_def\$ $pg12_def \$postgis_proc_upgrade_parsed_def\$;
  ELSIF $last_updated > version_from_num OR (
      $last_updated = version_from_num AND version_from_isdev
    ) FROM _postgis_upgrade_info()
  THEN
    EXECUTE 'DROP AGGREGATE IF EXISTS $aggsig';
    EXECUTE \$postgis_proc_upgrade_parsed_def\$ $def \$postgis_proc_upgrade_parsed_def\$;
  END IF;
END
\$postgis_proc_upgrade\$;
EOF
    }

    # This code handles operators by creating them if needed
    if (/^create operator\s+(\S+)\s*\(/i)
    {
        my $opname = $1;
        my $opleft = 'unknown';
        my $opright = 'unknown';
        my $def = $_;
        while(<INPUT>)
        {
            $def .= $_;
            $opleft = $1 if (/leftarg\s*=\s*(\w+)\s*,/i);
            $opright = $1 if (/rightarg\s*=\s*(\w+)\s*,/i);
            last if /\);/;
        }
        my $opsig = $opleft . " " . $opname . " " . $opright;

        my $last_updated = parse_last_updated($comment);
        if ( !$last_updated )
        {
            die "WARNING: no last updated info for operator '${opsig}'\n";
        }
        print "-- Operator ${opsig} -- LastUpdated: ${last_updated}\n";
        print <<"EOF";
DO LANGUAGE 'plpgsql'
\$postgis_proc_upgrade\$
BEGIN
  --IF $last_updated > version_from_num FROM _postgis_upgrade_info()
    --We trust presence of operator rather than version info
    IF NOT EXISTS (
        SELECT o.oprname
        FROM
            pg_catalog.pg_operator o,
            pg_catalog.pg_type tl,
            pg_catalog.pg_type tr
        WHERE
            o.oprleft = tl.oid AND
            o.oprright = tr.oid AND
            o.oprcode != 0 AND
            o.oprname = '$opname' AND
            tl.typname = '$opleft' AND
            tr.typname = '$opright'
    )
    THEN
    EXECUTE \$postgis_proc_upgrade_parsed_def\$ $def \$postgis_proc_upgrade_parsed_def\$;
  END IF;
END
\$postgis_proc_upgrade\$;
EOF
    }

    # Always output create ore replace view (see ticket #1097)
    if (/^create or replace view\s+(\S+)\s*/i)
    {
        print;
        while(<INPUT>)
        {
            print;
            last if /\;\s*$/;
        }
    }

    # Always output grant permissions (see ticket #3680)
    if (/^grant select\s+(\S+)\s*/i)
    {
        print;
        if ( !/\;\s*$/)
        {
            while(<INPUT>)
            {
                print;
                last if /\;\s*$/;
            }
        }
    }

    # Always output create ore replace rule
    if (/^create or replace rule\s+(\S+)\s*/i)
    {
        print;
        while(<INPUT>)
        {
            print;
            last if /\;\s*$/;
        }
    }

    # This code handles operator family by creating them if we are doing a major upgrade
    if (/^create operator family\s+(\w+)\s+USING\s+(\w+)\s*/i)
    {
        my $opfname = $1;
        my $amname = $2;
        my $def = $_;
        my $opfsig = $opfname . " " . $amname;
        while(<INPUT>)
        {
            $def .= $_;
            last if /\);/;
        }

        my $last_updated = parse_last_updated($comment);
        if ( !$last_updated )
        {
            die "WARNING: no last updated info for operator family '${opfname}'\n";
        }
        print "-- Operator family ${opfsig} -- LastUpdated: ${last_updated}\n";
        print <<"EOF";
DO LANGUAGE 'plpgsql'
\$postgis_proc_upgrade\$
BEGIN
  IF $last_updated > version_from_num FROM _postgis_upgrade_info() THEN
    EXECUTE \$postgis_proc_upgrade_parsed_def\$ $def \$postgis_proc_upgrade_parsed_def\$;
  END IF;
END
\$postgis_proc_upgrade\$;
EOF
    }

    # This code handles operator classes by creating them if we are doing a major upgrade
    if (/^create operator class\s+(\w+)\s*/i)
    {
        my $opclassname = $1;
        my $opctype = 'unknown';
        my $opcidx = 'unknown';
        my $def = $_;
        my $last_updated;
        my $subcomment = '';
        my @subobjects; # minversion, definition
        while(<INPUT>)
        {
            if (/^\s*\-\-/)
            {
                $subcomment .= $_;
                next;
            }

            $def .= $_;
            $opctype = $1 if (/for type (\w+) /i);
            $opcidx = $1 if (/using (\w+) /i);

            # Support adding members at later versions
            if (/\s+(OPERATOR|FUNCTION)\s+[0-9]+\s+ /)
            {
                my $last_updated = parse_last_updated($subcomment);
                if ($last_updated)
                {
                    my $subdefn = $_;
                    chop $subdefn;
                    $subdefn =~ s/[,;]$//; # strip ending comma or semicolon
                     # argument types must be specified in ALTER OPERATOR FAMILY
                    if ( $subdefn =~ m/\s+(OPERATOR.*)(FOR.*)/ )
                    {
                        $subdefn = $1.'('.$opctype.','.$opctype.') '.$2;
                    }
                    elsif ( $subdefn =~ m/\s+(OPERATOR.*)/ )
                    {
                        $subdefn = $1.'('.$opctype.','.$opctype.') ';
                    }
                    elsif ( $subdefn =~ m/\s+(FUNCTION\s+[0-9]+ )(.*)/ )
                    {
                        $subdefn = $1.'('.$opctype.','.$opctype.') '.$2;
                    }
                    push @subobjects, [$last_updated, $subdefn];
                }
                $subcomment = '';
            }
            last if /;$/;
        }
        $opctype =~ tr/A-Z/a-z/;
        $opcidx =~ tr/A-Z/a-z/;

        $last_updated = parse_last_updated($comment);
        if ( !$last_updated )
        {
            die "WARNING: no last updated info for operator class '${opclassname}'\n";
        }
        print "-- Operator class ${opclassname} -- LastUpdated: ${last_updated}\n";
        print <<"EOF";
DO LANGUAGE 'plpgsql'
\$postgis_proc_upgrade\$
BEGIN

  IF $last_updated > version_from_num FROM _postgis_upgrade_info()
  THEN
    EXECUTE \$postgis_proc_upgrade_parsed_def\$
    $def    \$postgis_proc_upgrade_parsed_def\$;
EOF
        my $ELSE="ELSE -- version_from >= $last_updated";
        for my $subobj (@subobjects)
        {
            $last_updated = @{$subobj}[0];
            $def = @{$subobj}[1];
            print <<"EOF";
  $ELSE
    -- Last Updated: ${last_updated}
    IF $last_updated > version_from_num FROM _postgis_upgrade_info() THEN
      EXECUTE \$postgis_proc_upgrade_parsed_def\$
        ALTER OPERATOR FAMILY ${opclassname} USING ${opcidx}
          ADD $def;
      \$postgis_proc_upgrade_parsed_def\$;
    END IF;
EOF
            $ELSE="";
        }
        print <<"EOF";
  END IF; -- version_from >= $last_updated
END
\$postgis_proc_upgrade\$;
EOF
    }

    $comment = '';
}

# If any deprecated function still exist, drop it now
my $deprecated_names = '{' . ( join ',', @renamed_deprecated_functions) . '}';
print <<"EOF";
-- Drop deprecated functions if possible
DO LANGUAGE 'plpgsql'
\$postgis_proc_upgrade\$
DECLARE
    deprecated_functions regprocedure[];
    new_name TEXT;
    rewrote_as_wrapper BOOLEAN;
    rec RECORD;
    extrec RECORD;
    procrec RECORD;
    sql TEXT;
    detail TEXT;
    hint TEXT;
BEGIN
    -- Fetch a list of deprecated functions

    SELECT array_agg(oid::regprocedure)
    FROM pg_catalog.pg_proc
    WHERE proname = ANY ('${deprecated_names}'::name[])
    INTO deprecated_functions;

    RAISE DEBUG 'Handling deprecated functions: %', deprecated_functions;

--    -- Rewrite views using deprecated functions
--    -- to improve the odds of being able to drop them
--
--    FOR rec IN
--        SELECT n.nspname AS schemaname,
--            c.relname AS viewname,
--            pg_catalog.pg_get_userbyid(c.relowner) AS viewowner,
--            pg_catalog.pg_get_viewdef(c.oid) AS definition,
--            CASE
--                WHEN 'check_option=cascaded' = ANY (c.reloptions) THEN 'WITH CASCADED CHECK OPTION'
--                WHEN 'check_option=local' = ANY (c.reloptions) THEN 'WITH LOCAL CHECK OPTION'
--                ELSE ''
--            END::text AS check_option
--        FROM pg_catalog.pg_class c
--        LEFT JOIN pg_catalog.pg_namespace n ON n.oid = c.relnamespace
--        WHERE c.relkind = 'v'
--        AND pg_catalog.pg_get_viewdef(c.oid) ~ 'deprecated_by_postgis'
--    LOOP
--        sql := pg_catalog.format('CREATE OR REPLACE VIEW %I.%I AS %s %s',
--            rec.schemaname,
--            rec.viewname,
--            pg_catalog.regexp_replace(rec.definition, '_deprecated_by_postgis_[^(]*', '', 'g'),
--            rec.check_option
--        );
--        RAISE NOTICE 'Updating view % to not use deprecated signatures', rec.viewname;
--        BEGIN
--            EXECUTE sql;
--        EXCEPTION
--        WHEN OTHERS THEN
--                GET STACKED DIAGNOSTICS detail := PG_EXCEPTION_DETAIL;
--                RAISE WARNING 'Could not rewrite view % using deprecated functions', rec.viewname
--                        USING DETAIL = pg_catalog.format('%s: %s', SQLERRM, detail);
--        END;
--    END LOOP;

    -- Try to drop all deprecated functions, or rewrite those
    -- who cannot be drop and rewrite them in SQL

    FOR rec IN SELECT pg_catalog.unnest(deprecated_functions) as proc
    LOOP --{

        RAISE DEBUG 'Handling deprecated function %', rec.proc;

        new_name := pg_catalog.regexp_replace(
            rec.proc::text,
            '_deprecated_by_postgis[^(]*\\(.*',
            ''
        );

        sql := pg_catalog.format('DROP FUNCTION %s', rec.proc);
        --RAISE DEBUG 'SQL: %', sql;
        BEGIN
            EXECUTE sql;
        EXCEPTION
        WHEN OTHERS THEN
            hint = 'Resolve the issue';
            GET STACKED DIAGNOSTICS detail := PG_EXCEPTION_DETAIL;
            IF detail LIKE '%view % depends%' THEN
                hint = pg_catalog.format(
                    'Replace the view changing all occurrences of %s in its definition with %s',
                    rec.proc,
                    new_name
                );
            END IF;
            hint = hint || ' and upgrade again';

            RAISE WARNING 'Deprecated function % left behind: %',
                rec.proc, SQLERRM
            USING DETAIL = detail, HINT = hint;

            --
            -- Try to rewrite the function as an SQL WRAPPER
            -- {
            SELECT pg_get_functiondef(oid) def, pronargs
            FROM pg_catalog.pg_proc WHERE oid = rec.proc
            INTO procrec;
            --
            -- TODO: don't even try if it's an aggregate or windowing
            --       function (procrec.prokind)
            -- TODO: don't even try if it's a scripting language function
            --       function (procrec.prokind)
            --
            -- Force LANGUAGE to be SQL
            sql := pg_catalog.regexp_replace(procrec.def, 'LANGUAGE [^ \n]*', 'LANGUAGE sql');
            --RAISE DEBUG 'SQL (LANGUAGE): %', sql;
            -- Change body to be a wrapper
            sql := pg_catalog.regexp_replace(
                sql,
                -- Find a stricted match here ?
                'AS .*',
                pg_catalog.format(
                    -- TODO: have the function raise a warning too ?
                    'AS \$\$ SELECT %s(%s) \$\$',
                    new_name,
                    (
                        SELECT array_to_string(
                            array_agg('\$' || x),
                            ','
                        )
                        FROM generate_series(1, procrec.pronargs) x
                    )
                )
            );
            RAISE DEBUG 'SQL: %', sql;
            rewrote_as_wrapper := false;
            BEGIN
                EXECUTE sql;
                rewrote_as_wrapper := true;
            EXCEPTION
            WHEN OTHERS THEN
                RAISE WARNING
                    'Deprecated function % could not be rewritten as a wrapper: % (%)',
                    rec.proc, SQLERRM, SQLSTATE;
            END;
            --
            --}


            -- Drop the function from any extension it is part of
            -- so dump/reloads still work
            FOR extrec IN
                SELECT e.extname
                FROM
                    pg_catalog.pg_extension e,
                    pg_catalog.pg_depend d
                WHERE
                    d.refclassid = 'pg_catalog.pg_extension'::pg_catalog.regclass AND
                    d.refobjid = e.oid AND
                    d.classid = 'pg_catalog.pg_proc'::pg_catalog.regclass AND
                    d.objid = rec.proc::oid
            LOOP
                RAISE DEBUG 'Unpackaging % from extension %', rec.proc, extrec.extname;
                sql := pg_catalog.format('ALTER EXTENSION %I DROP FUNCTION %s', extrec.extname, rec.proc);
                EXECUTE sql;
            END LOOP;
        END;
    END LOOP; --}
END
\$postgis_proc_upgrade\$;
EOF

close(INPUT);

print "DROP FUNCTION _postgis_upgrade_info();\n";

#print "COMMIT;\n";

1;

__END__

DO $$
DECLARE
    old_scripts text;
    new_scripts text;
    old_ver_int int[];
    new_ver_int int[];
    old_maj text;
    new_maj text;
    postgis_upgrade_info RECORD;
    postgis_upgrade_info_func_code TEXT;
BEGIN

    old_scripts := MODULE_scripts_installed();
    new_scripts := 'NEWVERSION';

    BEGIN
        new_ver_int := pg_catalog.string_to_array(
            pg_catalog.regexp_replace(
                new_scripts,
                '[^\d.].*',
                ''
            ),
            '.'
        )::int[];
    EXCEPTION WHEN OTHERS THEN
        RAISE EXCEPTION 'Cannot parse new version % into integers', new_scripts;
    END;

    BEGIN
        old_ver_int := pg_catalog.string_to_array(
            pg_catalog.regexp_replace(
                old_scripts,
                '[^\d.].*',
                ''
            ),
            '.'
        )::int[];
    EXCEPTION WHEN OTHERS THEN
        RAISE EXCEPTION 'Cannot parse old version % into integers', old_scripts;
    END;

    -- Guard against downgrade
    IF new_ver_int < old_ver_int
    THEN
        RAISE EXCEPTION 'Downgrade of MODULE from version % to version % is forbidden', old_scripts, new_scripts;
    END IF;


    -- Check for hard-upgrade being required
    SELECT into old_maj pg_catalog.substring(old_scripts, 1, 1);
    SELECT into new_maj pg_catalog.substring(new_scripts, 1, 1);

    -- 2.x to 3.x was upgrade-compatible, see
    -- https://trac.osgeo.org/postgis/ticket/4170#comment:1
    IF new_maj = '3' AND old_maj = '2' THEN
        old_maj = '3'; -- let's pretend old major = new major
    END IF;

    IF old_maj != new_maj THEN
        RAISE EXCEPTION 'Upgrade of MODULE from version % to version % requires a dump/reload. See PostGIS manual for instructions', old_scripts, new_scripts;
    END IF;

    WITH versions AS (
      SELECT 'NEWVERSION'::text as upgraded,
      MODULE_scripts_installed() as installed
    ) SELECT
      upgraded as scripts_upgraded,
      installed as scripts_installed,
      pg_catalog.substring(upgraded, '([0-9]+)\.')::int * 100 +
      pg_catalog.substring(upgraded, '[0-9]+\.([0-9]+)(\.|$)')::int
        as version_to_num,
      pg_catalog.substring(installed, '([0-9]+)\.')::int * 100 +
      pg_catalog.substring(installed, '[0-9]+\.([0-9]+)(\.|$)')::int
        as version_from_num,
      installed ~ 'dev|alpha|beta'
        as version_from_isdev
      FROM versions INTO postgis_upgrade_info
    ;

    postgis_upgrade_info_func_code := pg_catalog.format($func_code$
        CREATE FUNCTION _postgis_upgrade_info(OUT scripts_upgraded TEXT,
                                              OUT scripts_installed TEXT,
                                              OUT version_to_num INT,
                                              OUT version_from_num INT,
                                              OUT version_from_isdev BOOLEAN)
        AS
        $postgis_upgrade_info$
        BEGIN
            scripts_upgraded := %L :: TEXT;
            scripts_installed := %L :: TEXT;
            version_to_num := %L :: INT;
            version_from_num := %L :: INT;
            version_from_isdev := %L :: BOOLEAN;
            RETURN;
        END
        $postgis_upgrade_info$ LANGUAGE 'plpgsql' IMMUTABLE;
        $func_code$,
        postgis_upgrade_info.scripts_upgraded,
        postgis_upgrade_info.scripts_installed,
        postgis_upgrade_info.version_to_num,
        postgis_upgrade_info.version_from_num,
        postgis_upgrade_info.version_from_isdev);
    RAISE DEBUG 'Creating function %', postgis_upgrade_info_func_code;
    EXECUTE postgis_upgrade_info_func_code;
END
$$
LANGUAGE 'plpgsql';

