#!/usr/bin/perl

#
# PostGIS - Spatial Types for PostgreSQL
# http://postgis.refractions.net
#
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
# Conditionally upgraded types and operators. Only include these
# if the major numbers in version_from are less than the version_to
# number.
#
my $objs = {
 	"104" => { 
		"types" => {
			"box3d_extent" => 1,
			"pgis_abs" => 1
		}
	},
 	"105" => { 
		"operators" => {
			"geography >" => 1,
			"geography >=" => 1,
			"geography =" => 1,
			"geography <=" => 1,
			"geography <" => 1,
			"geography &&" => 1 
		},
		"opclasses" => {
			"gist_geography_ops" => 1,
			"btree_geography_ops" => 1
		},
		"views" => {
			"geography_columns" => 1
		},
		"types" => {
			"geography" => 1,
			"gidx" => 1
		}
	}
};

#
# Commandline argument handling
#
($#ARGV == 0) ||
die "Usage: perl postgis_proc_upgrade.pl <postgis.sql> <version_from> [<schema>]\nCreates a new SQL script to upgrade all of the PostGIS functions.\n"
	if ( @ARGV < 1 || @ARGV > 3 );

my $sql_file = $ARGV[0];
my $version_to = "";
my $version_to_num = 0;
my $version_from = $ARGV[1];
my $version_from_num = 0;
my $schema = "";
$schema = $ARGV[2] if @ARGV > 2;

if ( $version_from =~ /^(\d+)\.(\d+)/ )
{
	$version_from_num = 100 * $1 + $2; 
}
else
{
	die "Version from number invalid, must be of form X.X\n";
}

die "Unable to open input SQL file $sql_file\n"
	if ( ! -f $sql_file );

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
				last;
	}
}
close(INPUT); 

die "Unable to locate target new version number in $sql_file\n"
 	if( ! $version_to );

if ( $version_to =~ /(\d+)\.(\d+)\..*/ )
{
	$version_to = $1 . "." . $2;
	$version_to_num = 100 * $1 + $2; 
}
else
{
	die "Version to number invalid, must be of form X.X.X\n";
}

print qq{
--
-- UPGRADE SCRIPT FROM PostGIS $version_from TO PostGIS $version_to
--

};

print "BEGIN;\n";
print "SET search_path TO $schema;\n" if $schema;

#
# Add in the conditional check function to ensure this script is
# not being applied to a major version update.
#
while(<DATA>)
{
	s/NEWVERSION/$version_to/g;
	print;
}

#
# Go through the SQL file and strip out objects that cannot be 
# applied to an existing, loaded database: types and operators
# and operator classes that have already been defined.
#
open( INPUT, $sql_file ) || die "Couldn't open file: $sql_file\n";
while(<INPUT>)
{

	next if ( /^\-\-/ );

	#
	# Allow through deprecations from postgis_drop.sql
	#
	print if ( /^drop function if exists/i );
	print if ( /^drop aggregate if exists/i );

	if ( /^create or replace function/i )
	{
		print $_;
		my $endfunc = 0;
		while(<INPUT>)
		{
			print $_;
			$endfunc = 1 if /^\s*(\$\$\s*)?LANGUAGE /;
			last if ( $endfunc && /\;/ );
		}
	}

	if ( /^create type (\w+)/i )
	{
		my $newtype = $1;
		my $def .= $_;
		while(<INPUT>)
		{
			$def .= $_;
			last if /\)/;
		}
		my $ver = $version_from_num + 1;
		while( $version_from_num < $version_to_num && $ver <= $version_to_num )
		{
			if( $objs->{$ver}->{"types"}->{$newtype} )
			{
				print $def;
				last;
			}
			$ver++;
		}
	}

	# This code handles casts by dropping and recreating them.
	if ( /^create cast\s+\(\s*(\w+)\s+as\s+(\w+)\)/i )
	{
		my $type1 = $1;
		my $type2 = $2;
		my $def = $_;
		print "DROP CAST IF EXISTS ($type1 AS $type2);\n";
		print $def;
	}

	# This code handles aggregates by dropping and recreating them.
	if ( /^create aggregate\s+(\S+)\s*\(/i )
	{
		my $aggname = $1;
		my $aggtype = 'unknown';
		my $def = $_;
		while(<INPUT>)
		{
			$def .= $_;
			$aggtype = $1 if ( /basetype\s*=\s*([^,]*)\s*,/i );
			last if /\);/;
		}
		print "DROP AGGREGATE IF EXISTS $aggname($aggtype);\n";
		print $def;
	}
	
	# This code handles operators by creating them if we are doing a major upgrade
	if ( /^create operator\s+(\S+)\s*\(/i )
	{
		my $opname = $1;
		my $optype = 'unknown';
		my $def = $_;
		while(<INPUT>)
		{
			$def .= $_;
			$optype = $1 if ( /leftarg\s*=\s*(\w+)\s*,/i );
			last if /\);/;
		}
		my $opsig = $optype . " " . $opname;
		my $ver = $version_from_num + 1;
		while( $version_from_num < $version_to_num && $ver <= $version_to_num )
		{
			if( $objs->{$ver}->{"operators"}->{$opsig} )
			{
				print $def;
				last;
			}
			$ver++;
		}
	}

	# Always output create ore replace view (see ticket #1097)
	if ( /^create or replace view\s+(\S+)\s*/i )
	{
		print;
		while(<INPUT>)
		{
			print;
			last if /\;\s*$/;
		}
	}

	# Always output create ore replace rule 
	if ( /^create or replace rule\s+(\S+)\s*/i )
	{
		print;
		while(<INPUT>)
		{
			print;
			last if /\;\s*$/;
		}
	}

	# This code handles operator classes by creating them if we are doing a major upgrade
	if ( /^create operator class\s+(\w+)\s*/i )
	{
		my $opclassname = $1;
		my $opctype = 'unknown';
		my $opcidx = 'unknown';
		my $def = $_;
		while(<INPUT>)
		{
			$def .= $_;
			$opctype = $1 if ( /for type (\w+) /i );
			$opcidx = $1 if ( /using (\w+) /i );
			last if /\);/;
		}
		$opctype =~ tr/A-Z/a-z/;
		$opcidx =~ tr/A-Z/a-z/;
		my $ver = $version_from_num + 1;
		while( $version_from_num < $version_to_num && $ver <= $version_to_num )
		{
			if( $objs->{$ver}->{"opclasses"}->{$opclassname} )
			{
				print $def;
				last;
			}
			$ver++;
		}
	}
}

close( INPUT );

print "COMMIT;\n";

1;

__END__

CREATE OR REPLACE FUNCTION postgis_major_version_check()
RETURNS text
AS '
DECLARE
	old_scripts text;
	new_scripts text;
	old_maj text;
	new_maj text;
BEGIN
	--
	-- This uses postgis_lib_version() rather then
	-- postgis_scripts_installed() as in 1.0 because
	-- in the 1.0 => 1.1 transition that would result
	-- in an impossible upgrade:
	--
	--   from 0.3.0 to 1.1.0
	--
	-- Next releases will still be ok as
	-- postgis_lib_version() and postgis_scripts_installed()
	-- would both return actual PostGIS release number.
	-- 
	SELECT into old_scripts postgis_lib_version();
	SELECT into new_scripts ''NEWVERSION'';
	SELECT into old_maj substring(old_scripts from 1 for 2);
	SELECT into new_maj substring(new_scripts from 1 for 2);

	IF old_maj != new_maj THEN
		RAISE EXCEPTION ''Upgrade from version % to version % requires a dump/reload. See PostGIS manual for instructions'', old_scripts, new_scripts;
	ELSE
		RETURN ''Scripts versions checked for upgrade: ok'';
	END IF;
END
'
LANGUAGE 'plpgsql';

SELECT postgis_major_version_check();

DROP FUNCTION postgis_major_version_check();
