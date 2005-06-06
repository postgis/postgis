#/bin/sh

eval "exec perl -w $0 $@"
	if (0);

use strict;

($#ARGV == 0) || die "Usage: perl postgis_funxupgrade.pl <postgis.sql>\nCreates a new SQL script to upgrade all of the PostGIS functions.\n";

my $NEWVERSION = "UNDEF";

print "BEGIN;\n";

open( INPUT, $ARGV[0] ) || die "Couldn't open file: $ARGV[0]\n";

FUNC:
while(<INPUT>)
{
	if (m/^create or replace function postgis_scripts_installed()/i)
	{
		while(<INPUT>)
		{
			if ( m/SELECT .'(\d\.\d\.\d).'::text/i )
			{
				$NEWVERSION = $1;
				last FUNC;
			}
		}
	}
}

while(<DATA>)
{
	s/NEWVERSION/$NEWVERSION/g;
	print;
}

close(INPUT); open( INPUT, $ARGV[0] ) || die "Couldn't open file: $ARGV[0]\n";
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
}

close( INPUT );

print "COMMIT;\n";

1;

__END__

CREATE OR REPLACE FUNCTION postgis_script_version_check()
RETURNS text
AS '
DECLARE
	old_scripts text;
	new_scripts text;
	old_majmin text;
	new_majmin text;
BEGIN
	SELECT into old_scripts postgis_scripts_installed();
	SELECT into new_scripts ''NEWVERSION'';
	SELECT into old_majmin substring(old_scripts from ''[^\.]*\.[^\.]*'');
	SELECT into new_majmin substring(new_scripts from ''[^\.]*\.[^\.]*'');

	IF old_majmin != new_majmin THEN
		RAISE EXCEPTION ''Scripts upgrade from version % to version % requires a dump/reload. See postgis manual for instructions'', old_scripts, new_scripts;
	ELSE
		RETURN ''Scripts versions checked for upgrade: ok'';
	END IF;
END
'
LANGUAGE 'plpgsql';

SELECT postgis_script_version_check();

DROP FUNCTION postgis_script_version_check();
