# perl create_undef.pl <postgis.sql>
# creates a new sql script to delete all the postgis functions et al.

($#ARGV == 0) || die "usage: perl create_undef.pl <postgis.sql>\ncreates a new sql script to delete all the postgis functions et al.";


# drops are in the following order:
#	1. Indexing system stuff
#	2. Meta datatables <not done>
#	3. Aggregates  - must be of type geometry
#	4. Operators   - must be of type op(geometry,geometry)
#	5. Functions
#	6. Types
#	7. Tables

@aggs =();
@funcs = ();
@types = ();
@ops = ();


open( INPUT,$ARGV[0]) || die "couldnt open file: $ARGV[0]";


	print "begin;\n";
	print "--- indexing meta table stuff\n";
	print "delete from pg_amproc where amopclaid = (select oid from pg_opclass where opcname = 'gist_geometry_ops');\n";
	print "delete from pg_amop where amopclaid = (select oid from pg_opclass where opcname = 'gist_geometry_ops');\n";
	print "delete from pg_opclass where opcname = 'gist_geometry_ops';\n";


	while( $line = <INPUT>)
	{
		
		if ($line =~ /^create function/i)
		{
			push (@funcs, $line);
		}
		if ($line =~ /^create operator/i)
		{
			push (@ops, $line);
		}
		if ($line =~ /^create AGGREGATE/i)
		{
			push (@aggs, $line);
		}
		if ($line =~ /^create type/i)
		{
			push (@types, $line);
		}
	}

	#now have all the info, do the deletions

	print "--- AGGREGATEs\n";

	foreach $agg (@aggs)
	{
		if ($agg =~ /create AGGREGATE ([^(]+)/i )
		{
			print "drop AGGREGATE $1 geometry;\n";
		}
		else
		{
			die "couldnt parse line: $line\n";
		}
	}

	print "--- operators\n";

	foreach $op (@ops)
	{
		if ($op =~ /create operator ([^(]+)/i )
		{
			print "drop operator $1 (geometry,geometry);\n";
		}
		else
		{
			die "couldnt parse line: $line\n";
		}
	}

	
	print "--- functions\n";

	foreach $fn (@funcs)
	{
		chomp($fn); chomp($fn);

		
		if ($fn =~ /create function ([^(]+)\(([^()]*)\)/i )
		{
			print "drop function $1 ($2);\n";
		}
		else
		{
			die "couldnt parse line: $line\n";
		}
	}

	print "--- types\n";

	foreach $type (@types)
	{
		if ($type =~ /create type ([^(]+)/i )
		{
			print "drop type $1;\n";
		}
		else
		{
			die "couldnt parse line: $line\n";
		}
	}

	print "----tables\n";
	print "drop table spatial_ref_sys;\n";
	print "drop table geometry_columns;\n";
	print "\n";


	print "end;\n";


close( INPUT );

1;

