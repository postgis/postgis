#!/usr/bin/perl

eval "exec perl -w $0 $@"
	if (0);

# perl create_undef.pl <postgis.sql>
# creates a new sql script to delete all the postgis functions et al.

($#ARGV == 1) || die "Usage: perl create_undef.pl <postgis.sql> <pgsql_version #>\nCreates a new SQL script to delete all the PostGIS functions.\n";

# drops are in the following order:
#	1. Indexing system stuff
#	2. Meta datatables <not done>
#	3. Aggregates 
#	3. Casts
#	4. Operators 
#	5. Functions
#	6. Types
#	7. Tables

my @aggs = ();
my @casts = ();
my @funcs = ();
my @types = ();
my @ops = ();

my $version = $ARGV[1];

print "BEGIN;\n";

if ( $version ge "73" ) 
{
	print "-- Drop index bindings from system tables\n";
	print "DROP OPERATOR CLASS gist_geometry_ops USING gist CASCADE;\n";
}
else 
{
	print "-- Drop index bindings from system tables\n";
	print "DELETE FROM pg_amproc WHERE amopclaid = (SELECT oid FROM pg_opclass WHERE opcname = 'gist_geometry_ops');\n";
	print "DELETE FROM pg_amop WHERE amopclaid = (SELECT oid FROM pg_opclass WHERE opcname = 'gist_geometry_ops');\n";
	print "DELETE FROM pg_opclass WHERE opcname = 'gist_geometry_ops';\n";
}


open( INPUT, $ARGV[0] ) || die "Couldn't open file: $ARGV[0]\n";

while( my $line = <INPUT>)
{
	$line =~ s/[\r\n]//g;
	push (@funcs, $line) if ($line =~ /^create function/i);
	push (@funcs, $line) if ($line =~ /^create or replace function/i);
	push (@ops, $line) if ($line =~ /^create operator.*\(/i);
	push (@aggs, $line) if ($line =~ /^create aggregate/i);
	push (@types, $line) if ($line =~ /^create type/i);
	push (@casts, $line) if ($line =~ /^create cast/i);
}

close( INPUT );

print "-- Drop all aggregates.\n";

foreach my $agg (@aggs)
{
	if ( $agg =~ /create aggregate\s*(\w+)\s*\(/i )
	{
		if ( $version eq "71" ) 
		{
			print "DROP AGGREGATE $1 geometry;\n";
		}
		else 
		{
			print "DROP AGGREGATE $1 ( geometry );\n";
		}
	}
	else
	{
		die "Couldn't parse line: $agg\n";
	}
}

print "-- Drop all operators.\n";

foreach my $op (@ops)
{
	if ($op =~ /create operator ([^(]+)/i )
	{
		if ( $version ge "73" ) 
		{
			print "DROP OPERATOR $1 (geometry,geometry) CASCADE;\n";
		}
		else
		{
			print "DROP OPERATOR $1 (geometry,geometry);\n";
		}
	}
	else
	{
		die "Couldn't parse line: $op\n";
	}
}

	
print "-- Drop all casts.\n";

foreach my $cast (@casts)
{
	if ($cast =~ /create cast\s*\((.+?)\)/i )
	{
		print "DROP CAST ($1);\n";
	}
	else
	{
		die "Couldn't parse line: $cast\n";
	}
}

print "-- Drop all functions.\n";

foreach my $fn (@funcs)
{
	if ($fn =~ /.* function ([^(]+)\((.*)\)/i )
	{
		my $fn_nm = $1;
		my $fn_arg = $2;

		if ( $version ge "73" )
		{
			if ( ! ( $fn_nm =~ /_in/i || $fn_nm =~ /_out/i || $fn_nm =~ /_recv/i || $fn_nm =~ /_send/i || $fn_nm =~ /_analyze/i ) ) 
			{
				print "DROP FUNCTION $fn_nm ($fn_arg) CASCADE;\n";
			} 
		}
		else
		{
			print "DROP FUNCTION $fn_nm ($fn_arg);\n";
		}
	}
	else
	{
		die "Couldn't parse line: $fn\n";
	}
}

print "-- Drop all types.\n";

foreach my $type (@types)
{
	if ($type =~ /create type (\w+)/i )
	{
		if ( $version ge "73" ) 
		{
			print "DROP TYPE $1 CASCADE;\n";
		}
		else 
		{
			print "DROP TYPE $1;\n";
		}
	}
	else
	{
		die "Couldn't parse line: $type\n";
	}
}

print "-- Drop all tables.\n";
print "DROP TABLE spatial_ref_sys;\n";
print "DROP TABLE geometry_columns;\n";
print "\n";

print "COMMIT;\n";

1;

