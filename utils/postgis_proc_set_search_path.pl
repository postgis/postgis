#!/usr/bin/perl -w

#
# PostGIS - Spatial Types for PostgreSQL
# http://postgis.net
#
# Copyright (C) 2016 Regina Obe <lr@pcorp.us>
#
# This is free software; you can redistribute and/or modify it under
# the terms of the GNU General Public Licence. See the COPYING file.
#

#
# This script produces an .sql file containing
# ALTER FUNCTION SET PATH calls for each function
# in postgis.sql

# Having postgis functions force the search path 
# to just where postgis is installed is needed
# so that functions that call other functions during 
# database restore, materialized view creation, foreign table calls
# will always be able to find the companion functions
#

eval "exec perl -w $0 $@"
	if (0);

use strict;
use warnings;

#
# Commandline argument handling
#
($#ARGV == 0) ||
die "Usage: perl postgis_proc_set_path.pl <postgis.sql> <version_from> [<schema>]\nCreates a new SQL script 
to set search path for all functions in input script file.\n"
	if ( @ARGV < 1 || @ARGV > 3 );

my $sql_file = $ARGV[0];
my $module = 'postgis';
my $soname = '';
my $version_to = "";
my $version_to_num = 0;
my $version_from = $ARGV[1];
my $version_from_num = 0;
my $schema = "";
$schema = $ARGV[2] if @ARGV > 2;

die "Unable to open input SQL file $sql_file\n"
	if ( ! -f $sql_file );

## Header of do	
print 'DO language plpgsql $$';
my $dofunc_start = <<"EOF";
DECLARE param_postgis_schema text;
BEGIN
-- check if PostGIS is already installed
param_postgis_schema = (SELECT n.nspname from pg_extension e join pg_namespace n on e.extnamespace = n.oid WHERE extname = 'postgis');

-- if in middle install, it will be the current_schema or what was there already
param_postgis_schema = COALESCE(param_postgis_schema, current_schema());

IF param_postgis_schema != current_schema() THEN
	EXECUTE 'set search_path TO ' || quote_ident(param_postgis_schema);
END IF;

-- PostGIS set search path of functions
EOF
print $dofunc_start;

#
# Search the SQL file for the target version number (the 
# version we are upgrading *to*.
#
open( INPUT, $sql_file ) || die "Couldn't open file: $sql_file\n";


print qq{
--
-- ALTER FUNCTION script
--

};

#print "BEGIN;\n";
print "SET search_path TO $schema;\n" if $schema;

#
# Go through the SQL file and find all functions
# for each create an ALTER FUNCTION statement
# to set the search_path to schema postgis is installed in
open( INPUT, $sql_file ) || die "Couldn't open file: $sql_file\n";
while(<INPUT>)
{

	if ( /^create or replace function([^\)]+)([\)]{0,1})/i )
	{
		my $funchead = $1; # contains function header except the end )
		my $endhead = 0;
		my $endfunchead = $2;
		my $search_path_safe = -1; # we can put a search path on it without disrupting spatial index use
		
		if ($2 eq ')') ## reached end of header
		{
			$endhead = 1;
		}
		
		if ( /((add|drop)[\_]*(geometry|overview|raster)|internal)/i){
			# can't put search_path on addgeometrycolumn or addrasterconstraints 
			# since table names are sometimes passed in non-qualified
			# also can't work on some functions that take internals
			$search_path_safe = 0; 
		}
		
		if ( /st_transform/i){
			# st_transform functions query spatial_ref_sys
			# so could fail in materialized views and spatial indexes,
			# though often all done in C
			$search_path_safe = 1; 
		}

		
		#raster folks decided to break their func head in multiple lines 
		# so we need to do this crazy thing
		if ($endhead != 1)
		{
			while(<INPUT>)
			{
				#look for expressions with no ( and optionally ending in )
				if ( /^([^\)]*)([\)]{0,1})/i )
				{
					$funchead .= $1;
					$endfunchead = $2;
					if ($2 eq ')') ## reached end of header
					{
						$endhead = 1;
						
					}
				}
				last if ( $endhead );
	
			}
		}
		#strip quoted , trips up the default strip
		$funchead =~ s/(',')+//ig;
		#strip off default args from the function header
		$funchead =~ s/(default\s+[A-Za-z\.\+\-0-9\'\[\]\:\s]*)//ig;
		
		#check to see if function is STRICT or c or plpgsql
		# we can't put search path on non-STRICT sql since search path breaks SQL inlining
		# breaking sql inlining will break use of spatial index
		my $endfunc = 0;
		while(<INPUT>)
		{
			$endfunc = 1 if /^\s*(\$\$\s*)?LANGUAGE /i;
			if ( $endfunc == 1 && $search_path_safe == -1 ){
				$search_path_safe = 1 if /LANGUAGE\s+[\']*(plpgsql)/i;
				$search_path_safe = 1 if /STRICT/i;
				#exclude C functions unless we've include, 
				# in most cases except ST_Transform
				# c functions don't call dependent functions or tables
				$search_path_safe = 0 if /LANGUAGE\s+[\']*(c)/i;
			}
			last if ( $endfunc && /\;/ );
		}

		
		if ($search_path_safe == 1)
		{
			print "EXECUTE 'ALTER FUNCTION $funchead $endfunchead SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';\n";
		}
	}

}

close( INPUT );

## End of DO
print 'END;';
print '$$;';

__END__
 
