#/bin/sh

#
# This script produces an .sql file containing
# CREATE OR REPLACE calls for each function
# in lwpostgis.sql
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

($#ARGV == 0) ||
die "Usage: perl postgis_proc_upgrade.pl <postgis.sql> [<schema>]\nCreates a new SQL script to upgrade all of the PostGIS functions.\n"
	if ( @ARGV < 1 || @ARGV > 2 );

my $NEWVERSION = "UNDEF";
my %newtypes = ( "box3d_extent", 1, "pgis_abs", 1 );

print "BEGIN;\n";

print "SET search_path TO $ARGV[1];\n" if @ARGV>1;

open( INPUT, $ARGV[0] ) || die "Couldn't open file: $ARGV[0]\n";

FUNC:
while(<INPUT>)
{
	#
	# Since 1.1.0 scripts/lib/release versions are the same
	#
	if (m/^create or replace function postgis_scripts_installed()/i)
	{
		while(<INPUT>)
		{
			if ( m/SELECT .'(\d\.\d\..*).'::text/i )
			{
				$NEWVERSION = $1;
				last FUNC;
			}
		}
	}
}

print "-- $NEWVERSION\n";

while(<DATA>)
{
	s/NEWVERSION/$NEWVERSION/g;
	print;
}

close(INPUT); 

open( INPUT, $ARGV[0] ) || die "Couldn't open file: $ARGV[0]\n";
while(<INPUT>)
{
	my $checkit = 0;
	if (m/^create or replace function/i)
	{
		$checkit = 1 if m/postgis_scripts_installed()/i;
		print $_;
		while(<INPUT>)
		{
			if ( $checkit && m/SELECT .'(\d\.\d\.\d).'::text/i )
			{
				$NEWVERSION = $1;
			}
			print $_;
			last if m/^\s*LANGUAGE '/;
		}
	}

	if (m/^create type (\S+)/i)
	{
		my $newtype = $1;
		print $_ if $newtypes{$newtype};
		while(<INPUT>)
		{
			print $_ if $newtypes{$newtype};
			last if m/\)/;
		}
	}


	# This code handles aggregates by dropping and recreating them.
	# The DROP would fail on aggregates as they would not exist
	# in old postgis installations, thus we avoid this until we
	# find a better strategy.

	if (/^create aggregate\s+(\S+)\s*\(/i)
	{
		my $aggname = $1;
		my $basetype = 'unknown';
		my $def = $_;
		while(<INPUT>)
		{
			$def .= $_;
			$basetype = $1 if (m/basetype\s*=\s*([^,]*)\s*,/i);
			last if m/\);/;
		}
		print "DROP AGGREGATE IF EXISTS $aggname($basetype);\n";
		print "$def";
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
		RAISE EXCEPTION ''Scripts upgrade from version % to version % requires a dump/reload. See postgis manual for instructions'', old_scripts, new_scripts;
	ELSE
		RETURN ''Scripts versions checked for upgrade: ok'';
	END IF;
END
'
LANGUAGE 'plpgsql';

SELECT postgis_major_version_check();

DROP FUNCTION postgis_major_version_check();
