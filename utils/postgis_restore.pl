#/bin/sh
#
# This script is aimed at restoring postgis data
# from a dumpfile produced by pg_dump -Fc
#
# Basically it will restore all but things created by
# the given postgis.sql.
# Before restoring, it will create and postgis-enable
# the target db.
#
# A particular attention must be given to the spatial_ref_sys
# and geometry_columns tables which are created and populated
# from the dump, not the postgis.sql file. When the new installation
# is agains pgsql7.5+ and dump from pre7.5 this script should probably
# drop statistic fields from that table.... currently not done.
# Also, when upgrading to pgsq8.1+ (from <8.1) the oid column of
# geometry_columns  will be dropped, while it is needed for
# postgis opearations.
#
# Issues:
#	o Some obsoleted functions would not be present in the
#	  postgis.sql, but will be found in the dump. Currently
#	  some are skipped, but some depend on the pg version
#	  so will issue an ERROR due to unavailability of
#	  corresponding C function in postgis lib.
#	 
# 	o This script could do less then it does, to allow users
#	  to further modify edited dump before feeding it to the
#	  restoring side.
#	
#
# Tested on:
#
#	pg_dump-734/pg734 => pg_restore-743/pg743
#	pg_dump-743/pg734 => pg_restore-743/pg743
#	pg_dump-743/pg743 => pg_restore-743/pg743
#	pg_dump-734/pg734 => pg_restore-800/pg800
#	pg_dump-743/pg734 => pg_restore-800/pg800
#	pg_dump-743/pg743 => pg_restore-800/pg800
#	pg_dump-800/pg800 => pg_restore-800/pg800
#
eval "exec perl -w $0 $@"
	if (0);

use strict;

(@ARGV >= 3) || die "Usage: postgis_restore.pl <postgis.sql> <db> <dump> [<createdb_options>]\nRestore a custom dump (pg_dump -Fc) of a postgis enabled database.\n";

my $DEBUG=1;

my %aggs = ();
my %casts = ();
my %funcs = ();
my %types = ();
my %opclass = ();
my %ops = ();


# Old aggregate functions we don't carry any more
$aggs{"accum"} = 1;
$aggs{"fastunion"} = 1;
$aggs{"mem_collect"} = 1;

# This are old postgis functions which might
# still be in a dump
my %obsoleted_function = (
	'linefromtext', 1,
	'linestringfromtext', 1,
	'mlinefromtext', 1,
	'multilinestringfromtext', 1,
	'mpolyfromtext', 1,
	'multipolygonfromtext', 1,
	'polyfromtext', 1,
	'polygonfromtext', 1,
	'pointfromtext', 1,
	'mpointfromtext', 1,
	'multipointfromtext', 1,
	'geomcollfromtext', 1,
	'geometryfromtext', 1,
	'geomfromtext', 1,
	'pointfromwkb(geometry, integer)', 1,
	'pointfromwkb(geometry)', 1,
	'linefromwkb(geometry, integer)', 1,
	'linefromwkb(geometry)', 1,
	'linestringfromwkb(geometry, integer)', 1,
	'linestringfromwkb(geometry)', 1,
	'polyfromwkb(geometry, integer)', 1,
	'polyfromwkb(geometry)', 1,
	'polygonfromwkb(geometry, integer)', 1,
	'polygonfromwkb(geometry)', 1,
	'mpointfromwkb(geometry, integer)', 1,
	'mpointfromwkb(geometry)', 1,
	'multipointfromwkb(geometry, integer)', 1,
	'multipointfromwkb(geometry)', 1,
	'multilinefromwkb(geometry, integer)', 1,
	'multilinefromwkb(geometry)', 1,
	'mlinefromwkb(geometry, integer)', 1,
	'mlinefromwkb(geometry)', 1,
	'mpolyfromwkb(geometry, integer)', 1,
	'mpolyfromwkb(geometry)', 1,
	'multipolyfromwkb(geometry, integer)', 1,
	'multipolyfromwkb(geometry)', 1,
	'geomcollfromwkb(geometry, integer)', 1,
	'geomcollfromwkb(geometry)', 1,
	'wkb_in', 1,
	'wkb_out', 1,
	'wkb_recv', 1,
	'wkb_send', 1,
	'postgisgistcostestimate', 1,
	'ggeometry_compress', 1,
	'ggeometry_picksplit', 1,
	'gbox_picksplit', 1,
	'ggeometry_union', 1,
	'gbox_union', 1,
	'ggeometry_same', 1,
	'gbox_same', 1,
	'rtree_decompress', 1,
	'ggeometry_penalty', 1,
	'gbox_penalty', 1,
	'geometry_union(geometry, geometry)', 1,
	'geometry_inter(geometry, geometry)', 1,
	'geometry_size', 1,
	'ggeometry_consistent', 1,
	'xmin(box2d)', 1,
	'ymin(box2d)', 1,
	'xmax(box2d)', 1,
	'ymax(box2d)', 1,
	'optimistic_overlap', 1,
	'unite_finalfunc', 1,
	'numb_sub_objs(geometry)', 1,
	'truly_inside(geometry, geometry)', 1,
	'jtsnoop', 1,
	'_st_asgml(integer, geometry, integer)', 1,
	'text(boolean)', 1,
	'st_text(boolean)', 1,
	'postgis_jts_version', 1,
	'build_histogram2d', 1,
	'create_histogram2d', 1,
	'estimate_histogram2d', 1,
	'explode_histogram2d', 1,
	'histogram2d_in', 1,
	'histogram2d_out', 1,
	'st_histogram2d_in', 1,
	'st_histogram2d_out', 1,
	'st_build_histogram2d', 1,
	'st_create_histogram2d', 1,
	'st_estimate_histogram2d', 1,
	'st_explode_histogram2d', 1
);

# This are old postgis operators which might
# still be in a dump
my %obsoleted_ops = (
	'>>,box2d,box2d', 1,
	'<<,box2d,box2d', 1,
	'&>,box2d,box2d', 1,
	'&<,box2d,box2d', 1,
	'&&,box2d,box2d', 1,
	'~=,box2d,box2d', 1,
	'~,box2d,box2d', 1,
	'@,box2d,box2d', 1
);

my $postgissql = $ARGV[0]; shift(@ARGV);
my $dbname = $ARGV[0]; shift(@ARGV);
my $dump = $ARGV[0]; shift(@ARGV);
my $createdb_opt = '';
my $dumplist=$dump.".list";
my $dumpascii=$dump.".ascii";

$createdb_opt = join(' ', @ARGV) if @ARGV;

print "postgis.sql is $postgissql\n";
print "dbname is $dbname\n";
print "dumpfile is $dump\n";
print "database creation options: $createdb_opt\n" if $createdb_opt;


#
# Canonicalize type names (they change between dump versions).
# Here we also strip schema qualification
#
sub
canonicalize_typename
{
	my $arg=shift;

	# Lower case
	$arg = lc($arg);

	# Trim whitespaces
	$arg =~ s/^ *//;
	$arg =~ s/ *$//;

	# Strip schema qualification
	#$arg =~ s/^public.//;
	$arg =~ s/^.*\.//;

	# Handle type name changes
	if ( $arg eq 'opaque' ) {
		$arg = 'internal';
	} elsif ( $arg eq 'boolean' ) {
		$arg = 'bool';
	} elsif ( $arg eq 'oldgeometry' ) {
		$arg = 'geometry';
	}

	# Timestamp with or without time zone
	if ( $arg =~ /timestamp .* time zone/ ) {
		$arg = 'timestamp';
	}

	return $arg;
}

#
# Scan postgis.sql
#
print "Scanning $postgissql\n"; 
open( INPUT, $postgissql ) || die "Couldn't open file: $postgissql\n";
while( my $line = <INPUT>)
{
	$line =~ s/[\r\n]//g;
	#print "LINE: $line\n";

	next if $line =~ /^ *--/;

	if ($line =~ /^ *create (or replace)? function ([^ ]*) *\((.*)\)/i)
	{
		my $name = lc($2);
		my @args = split(",", $3);
		my $geomfound = 0;
		for (my $i=0; $i<@args; $i++)
		{
			my $arg = lc($args[$i]);
			#print "ARG1: [$arg]\n";
			$arg =~ s/^ *//;
			$arg =~ s/ *$//;
			#print "ARG2: [$arg]\n";
			if ( $arg =~ /^int[48]?$/ ) {
				$args[$i] = 'integer';
				next;
			}
			if ( $arg eq 'float4' ) {
				$args[$i] = 'real';
				next;
			}
			if ( $arg eq 'float8' ) {
				$args[$i] = 'double precision';
				next;
			}
			if ( $arg eq 'varchar' ) {
				$args[$i] = 'character varying';
				next;
			}
			if ( $arg eq 'boolean' ) {
				$args[$i] = 'bool';
				next;
			}
			if ( $arg eq 'opaque' ) {
				$args[$i] = 'internal';
				next;
			}
			$args[$i] = $arg;
			$geomfound++ if ( $arg eq 'oldgeometry' );
		}
		my $id = $name."(".join(", ", @args).")";
		$funcs{$id} = 1;
		print "SQLFUNC: $id\n" if $DEBUG;
		if ( $geomfound )
		{
			for (my $i=0; $i<@args; $i++)
			{
				my $arg = $args[$i];
				$arg = 'geometry' if ($arg eq 'oldgeometry');
				$args[$i] = $arg;
			}
			my $id = $name."(".join(", ", @args).")";
			$funcs{$id} = 1;
			print "SQLFUNC: $id\n" if $DEBUG;
		}
		next;
	}
	if ($line =~ /^create type +([^ ]+)/i)
	{
		my $type = $1;
		$types{$type} = 1;
		print "SQLTYPE $type\n" if $DEBUG;
		if ( $type eq 'oldgeometry' )
		{
			$type = 'geometry';
			$types{$type} = 1;
			print "SQLTYPE $type\n" if $DEBUG;
		}
		next;
	}
	if ( $line =~ /^create aggregate *([^ ]*) *\(/i )
	{
		my $name = lc($1);
		$name =~ s/^public.//;
		my $type = undef;
		while( my $subline = <INPUT>)
		{
			if ( $subline =~ /basetype .* ([^, ]*)/i )
			{
				$type = $1;
				last;
			}
			last if $subline =~ /;[\t ]*$/;
		}
		if ( ! defined($type) )
		{
			print "Could not find base type for aggregate $name\n";
			print "($line)\n";
			exit 1;
		}
		my $id = $name.'('.$type.')';
		print "SQLAGG $id\n" if $DEBUG;
		$aggs{$id} = 1;
		if ( $type eq 'oldgeometry' )
		{
			$type = 'geometry';
			my $id = $name.'('.$type.')';
			$aggs{$id} = 1;
			print "SQLAGG $id\n" if $DEBUG;
		}
		next;
	}

	# CAST
	if ($line =~ /create cast *\( *([^ ]*) *as *([^ )]*) *\) *with function *([^ ]*) *\(([^ ]*) *\)/i)
	{
		my $from = canonicalize_typename($1);
		my $to = canonicalize_typename($2);
		my $funcname = canonicalize_typename($3);
		my $funcarg = canonicalize_typename($4);

		my $id = $funcarg.'.'.$funcname;
		$casts{$id} = 1;
		print "SQLFNCAST $id\n" if $DEBUG;

		$id = $from.','.$to;
		$casts{$id} = 1;
		print "SQLCAST $id\n" if $DEBUG;

		next;
	}

	# OPERATOR CLASS
	if ($line =~ /create operator class *([^ ]*)/i)
	{
		my $id = lc($1);
		print "SQLOPCLASS $id\n" if $DEBUG;
		$opclass{$id} = 1;
		next;
	}

	# OPERATOR 
	if ($line =~ /create operator *([^ ]*)/i)
	{
		my $name = ($1);
		my $larg = undef;
		my $rarg = undef;
		while( my $subline = <INPUT>)
		{
			last if $subline =~ /;[\t ]*$/;
			if ( $subline =~ /leftarg *= *([^ ,]*)/i )
			{
				$larg=lc($1);
			}
			if ( $subline =~ /rightarg *= *([^ ,]*)/i )
			{
				$rarg=lc($1);
			}
		}
		my $id = $name.','.$larg.','.$rarg;
		print "SQLOP $id\n" if $DEBUG;
		$ops{$id} = 1;
		if ( $larg eq 'oldgeometry' || $rarg eq 'oldgeometry' )
		{
			$larg = 'geometry' if $larg eq 'oldgeometry';
			$rarg = 'geometry' if $rarg eq 'oldgeometry';
			my $id = $name.','.$larg.','.$rarg;
			print "SQLOP $id\n" if $DEBUG;
			$ops{$id} = 1;
		}
		next;
	}
}
close( INPUT );
#exit;


#
# Scan dump list
#
print "Scanning $dump list\n"; 
open( OUTPUT, ">$dumplist") || die "Can't write to ".$dump.".list\n";
open( INPUT, "pg_restore -l $dump |") || die "Couldn't run pg_restore -l $dump\n";
while( my $line = <INPUT> )
{
	next if $line =~ /^;/;
	next if $line =~ /^ *--/;

	if ($line =~ / FUNCTION/)
	{
		my $funcname;
		my @args;

		#print "FUNCTION: [$line]\n";

		if ($line =~ / FUNCTION *([^ ]*) *\(([^)]*)\)/)
		{
			#print " matched <800\n";
			$funcname = $1;
			@args = split(",", $2);
		}
		elsif ($line =~ / FUNCTION *([^ ]+) *([^ ]+) *\(([^)]*)\)/)
		{
			#print " matched 800\n";
			$funcname = $2;
			@args = split(",", $3);
		}
		else
		{
			print " unknown FUNCTION match\n";
		}

		$funcname =~ s/^"//;
		$funcname =~ s/"$//;

		#print "  FUNCNAME: [$funcname]\n";
		#print "  ARGS: [".@args."]\n";

		my $wkbinvolved = 0;
		for (my $i=0; $i<@args; $i++)
		{
			my $arg = canonicalize_typename($args[$i]);
			$args[$i] = $arg;
			$wkbinvolved++ if ( $arg eq 'wkb' );
		}

		my $args = join(', ', @args);
		#print "ARGS SCALAR: [$args]\n";
		my $id = $funcname."(".$args.")";
		#print "ID: [$id]\n";

		# WKB type is obsoleted
		if ( $wkbinvolved )
		{
			print "SKIPPING FUNC $id\n" if $DEBUG;
			next;
		}

		if ( $funcname eq 'plpgsql_call_handler' )
		{
			print "SKIPPING FUNC $id\n" if $DEBUG;
			next;
		}

		if ( $funcname eq 'plpgsql_validator' )
		{
			print "SKIPPING FUNC $id\n" if $DEBUG;
			next;
		}

		if ( $obsoleted_function{$funcname} || $obsoleted_function{$id} )
		{
			print "SKIPPING OBSOLETED FUNC $id\n" if $DEBUG;
			next;
		}

		if ( $funcs{$id} )
		{
			print "SKIPPING PGIS FUNC $id\n" if $DEBUG;
			next;
		}
		print "KEEPING FUNCTION: [$id]\n" if $DEBUG;
		#next;
	}
	elsif ($line =~ / AGGREGATE ([^ ]* )?([^ ]*)\((.*)\)/)
	{
		my $name = $2;
		my @args = split(",", $3);
		for (my $i=0; $i<@args; $i++)
		{
			$args[$i] = canonicalize_typename($args[$i]);
		}
		my $args = join(', ', @args);
		my $id = $name."(".$args.")";
		if ( $aggs{$id} )
		{
			print "SKIPPING PGIS AGG $id\n" if $DEBUG;
			next;
		}
		# This is an old postgis aggregate
		if ( $name eq 'fastunion' )
		{
			print "SKIPPING old PGIS AGG $id\n" if $DEBUG;
			next;
		}

		# This is an old postgis aggregate
		if ( $name eq 'mem_collect' )
		{
			print "SKIPPING old PGIS AGG $id\n" if $DEBUG;
			next;
		}

		# This is an old postgis aggregate
		if ( $name eq 'accum' )
		{
			print "SKIPPING old PGIS AGG $id\n" if $DEBUG;
			next;
		}
		print "KEEPING AGGREGATE [$id]\n" if $DEBUG;
		#next;
	}
	elsif ($line =~ / TYPE ([^ ]+ )?([^ ]*) .*/)
	{
		my $type = canonicalize_typename($2);
		if ( $type eq 'wkb' )
		{
			print "SKIPPING PGIS TYPE $type\n" if $DEBUG;
			next;
		}
		if ( $types{$type} )
		{
			print "SKIPPING PGIS TYPE $type\n" if $DEBUG;
			next;
		}
		print "KEEPING TYPE [$type]\n" if $DEBUG;
		#next;
	}
	elsif ($line =~ / PROCEDURAL LANGUAGE (public )?plpgsql/)
	{
		print "SKIPPING PROCLANG plpgsql\n" if $DEBUG;
		next;
	}

	# spatial_ref_sys and geometry_columns
	elsif ($line =~ / TABLE geometry_columns/)
	{
		#print "SKIPPING geometry_columns schema\n" if $DEBUG;
		#next;
	}
	elsif ($line =~ / TABLE spatial_ref_sys/)
	{
		#print "SKIPPING spatial_ref_sys schema\n" if $DEBUG;
		#next;
	}

	#
	# pg_restore-7.4:
	# 354; 11038762 OPERATOR CLASS btree_geometry_ops strk
	#
	# pg_restore-8.0:
	# 354; 0 11038762 OPERATOR CLASS public btree_geometry_ops strk
	#
	elsif ($line =~ / OPERATOR CLASS +([^ ]+ )?([^ ]+) ([^ ]+)/)
	{
		my $id = lc($2);

		if ( $opclass{$id} )
		{
			print "SKIPPING PGIS OPCLASS $id\n" if $DEBUG;
			next;
		}
		print "KEEPING OPCLASS [$id]\n" if $DEBUG;
	}

	# casts were implicit in PG72 
	elsif ($line =~ / CAST /)
	{

		my $arg1=undef;
		my $arg2=undef;

		#
		# CAST def by pg_restore 80,81 on pg_dump 73
		#
		# 734_800; 0 00000 CAST public box2d (public.box3d) 
		# 734_810; 0 00000 CAST public box2d (public.box3d) 
		#
		if ($line =~ / CAST *([^ ]+) ([^ ]+) *\( *([^ )]+) *\)/)
		{
			$arg1 = canonicalize_typename($3);
			$arg2 = canonicalize_typename($2);
		}

		#
		# CAST def by pg_restore 73,74 on pg_dump 73
		#
		# 734_743; 00000 CAST box2d (public.box3d) 
		# 734_734; 00000 CAST box2d (public.box3d) 
		#
		elsif ($line =~ / CAST *([^ ]*) *\( *([^ )]*) *\)/)
		{
			$arg1 = canonicalize_typename($2);
			$arg2 = canonicalize_typename($1);
		}

		#
		# CAST def by pg_restore 81 on pg_dump 81
		#
		# 810_810; 0000 00000 CAST pg_catalog CAST (boolean AS text) 
		#
		elsif ($line =~ / CAST [^ ]* CAST \(([^ ]*) AS ([^ )]*)\)/)
		{
			$arg1 = canonicalize_typename($1);
			$arg2 = canonicalize_typename($2);
		}

		if (defined($arg1) && defined($arg2))
		{
			my $id = $arg1.",".$arg2;
			if ( $casts{$id} )
			{
				print "SKIPPING PGIS CAST $id\n" if $DEBUG;
				next;
			}
			if ($arg1 eq 'wkb' || $arg2 eq 'wkb')
			{
				print "SKIPPING PGIS CAST $id\n" if $DEBUG;
				next;
			}
			print "KEEPING CAST $id (see CAST)\n" if $DEBUG;
		}
		else
		{
			print "KEEPING CAST (unknown def): $line\n";
		}

	} # CAST

	print OUTPUT $line;
#	print "UNHANDLED: $line"
}
close( INPUT );
close(OUTPUT);

print "Producing ascii dump $dumpascii\n"; 
open( INPUT, "pg_restore -L $dumplist $dump |") || die "Can't run pg_restore\n";
open( OUTPUT, ">$dumpascii") || die "Can't write to $dumpascii\n";
while( my $line = <INPUT> )
{
	next if $line =~ /^ *--/;

	if ( $line =~ /^SET search_path/ )
	{
		$line =~ s/; *$/, public;/; 
	}

	elsif ( $line =~ /OPERATOR CLASS /)
	{
	}

	elsif ( $line =~ /CREATE OPERATOR *([^ ,]*)/)
	{
		my $name = canonicalize_typename($1);
		my $larg = undef;
		my $rarg = undef;
		my @sublines = ($line);
		while( my $subline = <INPUT>)
		{
			push(@sublines, $subline);
			last if $subline =~ /;[\t ]*$/;
			if ( $subline =~ /leftarg *= *([^ ,]*)/i )
			{
				$larg=canonicalize_typename($1);
			}
			if ( $subline =~ /rightarg *= *([^ ,]*)/i )
			{
				$rarg=canonicalize_typename($1);
			}
		}

		my $id = $name.','.$larg.','.$rarg;

		if ( $obsoleted_ops{$id} )
		{
			print "SKIPPING OBSOLETED FUNC $id\n" if $DEBUG;
			next;
		}

		if ( $ops{$id} )
		{
			print "SKIPPING PGIS OP $id\n" if $DEBUG;
			next;
		}

		print "KEEPING OP $id\n" if $DEBUG;
		print OUTPUT @sublines;
		next;
	}

	print OUTPUT $line;
	# TODO:
	#  skip postgis operator, checking for basetype
	#  when implemented operators skip must be disabled
	#  in the first scan of ToC
}
close(INPUT) || die "pg_restore call failed\n";
close(OUTPUT);

#exit(1);

#
# Create the new db and install plpgsql language
#
print "Creating db ($dbname)\n";
`createdb $createdb_opt $dbname`;
die "Database creation failed\n" if ($?);
print "Adding plpgsql\n";
`createlang $createdb_opt plpgsql $dbname`;

#
# Open a pipe to the SQL monitor
#
open( PSQL, "| psql $createdb_opt -a $dbname") || die "Can't run psql\n";

#
# Source new postgis.sql
#
print "Sourcing $postgissql\n";
open(INPUT, "<$postgissql") || die "Can't read $postgissql\n";
while(<INPUT>) { print PSQL; }
close(INPUT);

#
# Drop geometry_columns and spatial_ref_sys
# (we want version from the dump)
#
print "Dropping geometry_columns and spatial_ref_sys\n";
print PSQL "DROP TABLE geometry_columns;";
print PSQL "DROP TABLE spatial_ref_sys;";
#print "Now source $dumpascii manually\n";
#exit(1);

#
# Source modified ascii dump
#
print "Restoring ascii dump $dumpascii\n";
open(INPUT, "<$dumpascii") || die "Can't read $postgissql\n";
while(<INPUT>) { print PSQL; }
close(INPUT);
close(PSQL) || die "psql run failed\n"
