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

eval "exec perl $0 $@"
	if (0);

(@ARGV == 3) || die "Usage: postgis_restore.pl <postgis.sql> <db> <dump>\nRestore a custom dump (pg_dump -Fc) of a postgis enabled database.\n";

$DEBUG=1;

my %aggs = {};
my %fncasts = ();
my %casts = ();
my %funcs = {};
my %types = {};
my %opclass = {};
my %ops = {};

my $postgissql = $ARGV[0];
my $dbname = $ARGV[1];
my $dump = $ARGV[2];
my $dumplist=$dump.".list";
my $dumpascii=$dump.".ascii";

print "postgis.sql is $postgsisql\n";
print "dbname is $dbname\n";
print "dumpfile is $dump\n";

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
			$arg = lc($args[$i]);
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
				$arg = $args[$i];
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
	if ($line =~ /^create aggregate *([^ ]*) *\(/i)
	{
		my $name = lc($1);
		my $type = undef;
		while( my $subline = <INPUT>)
		{
			if ( $subline =~ /basetype .* ([^, ]*)/ )
			{
				$type = $1;
				last;
			}
			last if $subline =~ /;[\t ]*$/;
		}
		if ( $type eq undef )
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
		my $from = lc($1);
		my $to = lc($2);
		my $funcname = lc($3);
		my $funcarg = lc($4);

		my $id = $funcname."(".$funcarg.")";
		$fncasts{$id} = 1;
		print "SQLFNCAST $id\n" if $DEBUG;
		if ( $funcarg eq 'oldgeometry' )
		{
			$funcarg = 'geometry';
			my $id = $funcname."(".$funcarg.")";
			$fncasts{$id} = 1;
			print "SQLFNCAST $id\n" if $DEBUG;
		}

		my $id = $from.','.$to;
		$casts{$id} = 1;
		print "SQLCAST $id\n" if $DEBUG;
		if ( $from eq 'oldgeometry' || $to eq 'oldgeometry' )
		{
			$from = 'geometry' if $from eq 'geometry';
			$to = 'geometry' if $to eq 'geometry';
			my $id = $from.','.$to;
			$casts{$id} = 1;
			print "SQLCAST $id\n" if $DEBUG;
		}


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

	if ($line =~ / FUNCTION *([^ ]*) *\(([^)]*)\)/)
	{
		my $funcname = $1;
		#print "FUNCNAME: [$funcname]\n";
		my @args = split(",", $2);
		#print "ARGS: [".@args."]\n";
		my $wkbinvolved = 0;
		for (my $i=0; $i<@args; $i++)
		{
			$arg = lc($args[$i]);
			$arg =~ s/^ *//;
			$arg =~ s/ *$//;
			$arg =~ s/^public.//;
			if ( $arg eq 'opaque' ) {
				$args[$i] = 'internal';
				next;
			}
			$args[$i] = $arg;
			$wkbinvolved++ if ( $arg eq 'wkb' );
		}

		$args = join(', ', @args);
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
		# This is an old postgis function which might
		# still be in a dump
		if ( $funcname eq 'unite_finalfunc' )
		{
			print "SKIPPING FUNC $id\n" if $DEBUG;
			next;
		}

		# This are old postgis functions which might
		# still be in a dump
		if ( $funcname eq 'postgisgistcostestimate' )
		{
			print "SKIPPING FUNC $id\n" if $DEBUG;
			next;
		}
		if ( $funcname eq 'wkb_in' || $funcname eq 'wkb_out' )
		{
			print "SKIPPING FUNC $id\n" if $DEBUG;
			next;
		}
		if ( $funcname eq 'ggeometry_consistent' ||
			$funcname eq 'ggeometry_compress' ||
			$funcname eq 'ggeometry_picksplit' ||
			$funcname eq 'gbox_picksplit' ||
			$funcname eq 'ggeometry_union' ||
			$funcname eq 'gbox_union' ||
			$funcname eq 'ggeometry_same' ||
			$funcname eq 'gbox_same' ||
			$funcname eq 'rtree_decompress' ||
			$funcname eq 'ggeometry_penalty' ||
			$funcname eq 'gbox_penalty' ||
			$id eq 'geometry_union(geometry, geometry)' ||
			$id eq 'geometry_inter(geometry, geometry)' ||
			$funcname eq 'geometry_size' )
		{
			print "SKIPPING FUNC $id\n" if $DEBUG;
			next;
		}

		if ( $id eq 'create_histogram2d(box3d, integer)' ||
			$id eq 'estimate_histogram2d(histogram2d, box)' )
		{
			print "SKIPPING FUNC $id\n" if $DEBUG;
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
	elsif ($line =~ / AGGREGATE (.*)\((.*)\)/)
	{
		my $name = $1;
		my @args = split(",", $2);
		for (my $i=0; $i<@args; $i++)
		{
			$arg = lc($args[$i]);
			$arg =~ s/^ *//;
			$arg =~ s/ *$//;
			$arg =~ s/^public.//;
			if ( $arg eq 'opaque' ) {
				$args[$i] = 'internal';
				next;
			}
			$args[$i] = $arg;
		}
		$args = join(', ', @args);
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

		print "KEEPING AGGREGATE [$id]\n" if $DEBUG;
		#next;
	}
	elsif ($line =~ / TYPE (.*) .*/)
	{
		my $type = lc($1);
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
	elsif ($line =~ / PROCEDURAL LANGUAGE plpgsql/)
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

	elsif ($line =~ / OPERATOR CLASS *([^ ]*)/)
	{
		my $id = lc($1);

		if ( $opclass{$id} )
		{
			print "SKIPPING PGIS OPCLASS $id\n" if $DEBUG;
			next;
		}
		print "KEEPING OPCLASS [$id]\n" if $DEBUG;
	}

	# CAST def by pg73
	elsif ($line =~ / CAST *([^ ]*) *\( *([^ )]*) *\)/)
	{
		my $arg1 = lc($1);
		my $arg2 = lc($2);
		$arg1 =~ s/^public\.//;
		$arg2 =~ s/^public\.//;
		my $id = $arg1."(".$arg2.")";
		if ( $fncasts{$id} )
		{
			print "SKIPPING PGIS FNCAST $id\n" if $DEBUG;
			next;
		}
		#if ($arg1 eq 'box3d' || $arg2 eq 'geometry')
		#{
			#print "SKIPPING PGIS FNCAST $id\n" if $DEBUG;
			#next;
		#}
		if ($arg1 eq 'wkb' || $arg2 eq 'wkb')
		{
			print "SKIPPING PGIS FNCAST $id\n" if $DEBUG;
			next;
		}
		print "KEEPING FNCAST $id (see CAST)\n" if $DEBUG;
	}

	# CAST def by pg74
	elsif ($line =~ / CAST CAST *\(([^ ]*) *AS *([^ )]*) *\)/)
	{
		my $arg1 = lc($1);
		my $arg2 = lc($2);
		$arg1 =~ s/^public\.//;
		$arg2 =~ s/^public\.//;
		my $id = $arg1.",".$arg2;
		if ( $casts{$id} )
		{
			print "SKIPPING PGIS CAST $id\n" if $DEBUG;
			next;
		}
		#if ($arg1 eq 'box3d' || $arg2 eq 'geometry')
		#{
			#print "SKIPPING PGIS CAST $id\n" if $DEBUG;
			#next;
		#}
		if ($arg1 eq 'wkb' || $arg2 eq 'wkb')
		{
			print "SKIPPING PGIS CAST $id\n" if $DEBUG;
			next;
		}
		print "KEEPING CAST $id\n" if $DEBUG;
	}
	print OUTPUT $line;
#	print "UNANDLED: $line"
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
		my $name = lc($1);
		$name =~ s/^.*\.//;
		my $larg = undef;
		my $rarg = undef;
		my @sublines = ($line);
		while( my $subline = <INPUT>)
		{
			push(@sublines, $subline);
			last if $subline =~ /;[\t ]*$/;
			if ( $subline =~ /leftarg *= *([^ ,]*)/i )
			{
				$larg=lc($1);
				$larg =~ s/^.*\.//;
			}
			if ( $subline =~ /rightarg *= *([^ ,]*)/i )
			{
				$rarg=lc($1);
				$rarg =~ s/^.*\.//;
			}
		}
		my $id = $name.','.$larg.','.$rarg;
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
close(INPUT);
close(OUTPUT);

#exit(1);

#
# Create the new db and install plpgsql language
#
print "Creating db ($dbname)\n";
`createdb $dbname`;
print "Adding plpgsql\n";
`createlang plpgsql $dbname`;

#
# Open a pipe to the SQL monitor
#
open( PSQL, "| psql -a $dbname") || die "Can't run psql\n";

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
close(PSQL);
