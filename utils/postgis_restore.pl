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

eval "exec perl $0 $@"
	if (0);

(@ARGV == 3) || die "Usage: postgis_restore.pl <postgis.sql> <db> <dump>\nRestore a custom dump (pg_dump -Fc) of a postgis enabled database.\n";

$DEBUG=1;

my %aggs = {};
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
	if ($line =~ /^ *create (or replace)? function ([^ ]*) *\((.*)\)/i)
	{
		my $name = lc($2);
		my @args = split(",", $3);
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
		}
		my $id = $name."(".join(", ", @args).")";
		$funcs{$id} = 1;
		print "SQLFUNC: $id\n" if $DEBUG;
		next;
	}
	if ($line =~ /^create type +([^ ]+)/i)
	{
		my $type = $1;
		print "SQLTYPE $type\n" if $DEBUG;
		$types{$type} = 1;
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
			last if $subline =~ /\);/;
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
		next;
	}
	if ($line =~ /create cast .* with function *([^ ]*) *\(([^ ]*) *\)/i)
	{
		my $id = lc($1)."(".lc($2).")";
		print "SQLCAST $id\n" if $DEBUG;
		$casts{$id} = 1;
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
			last if $subline =~ /\);/;
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

	if ($line =~ / FUNCTION (.*) \((.*)\)/)
	{
		my $funcname = $1;
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
		my $id = $funcname."(".$args.")";
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
		print "KEEPING AGGREGATE [$id]\n" if $DEBUG;
		#next;
	}
	elsif ($line =~ / TYPE (.*) .*/)
	{
		my $type = $1;
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

	elsif ($line =~ / CAST *([^ ]*) *\( *([^ )]*) *\)/)
	{
		my $arg1 = lc($1);
		my $arg2 = lc($2);
		$arg1 =~ s/^public\.//;
		$arg2 =~ s/^public\.//;
		my $id = $arg1."(".$arg2.")";
		if ( $casts{$id} )
		{
			print "SKIPPING PGIS CAST $id\n" if $DEBUG;
			next;
		}
		if ($arg1 eq 'box3d' || $arg2 eq 'geometry')
		{
			print "SKIPPING PGIS type CAST $id\n" if $DEBUG;
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

	elsif ( $line =~ /OPERATOR *([^ ,]*)/)
	{
		my $name = $1;
		my $larg = undef;
		my $rarg = undef;
		my @sublines = ($line);
		while( my $subline = <INPUT>)
		{
			last if $subline =~ /\);/;
			if ( $subline =~ /leftarg *= *([^ ,]*)/i )
			{
				$larg=lc($1);
				$larg =~ s/^public\.//;
			}
			if ( $subline =~ /rightarg *= *([^ ,]*)/i )
			{
				$rarg=lc($1);
				$rarg =~ s/^public\.//;
			}
			push(@sublines, $subline);
		}
		my $id = $name.','.$larg.','.$rarg;
		if ( $ops{$id} )
		{
			print "SKIPPING PGIS OP $id\n" if $DEBUG;
			next;
		}
		print "KEEPING OP $id\n" if $DEBUG;
		print OUTPUT @sublines;
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
print "Creating db ($dbname)\n";
`createdb $dbname`;
print "Adding plpgsql\n";
`createlang plpgsql $dbname`;

open( PSQL, "| psql $dbname") || die "Can't run psql\n";
print "Sourcing $postgissql\n";
#`psql -f $postgissql $dbname`;
open(INPUT, "<$postgissql") || die "Can't read $postgissql\n";
while(<INPUT>) { print PSQL; }
close(INPUT);
print "Dropping geometry_columns and spatial_ref_sys\n";
#`psql -c "drop table geometry_columns; drop table spatial_ref_sys;" $dbname`;
print PSQL "DROP TABLE geometry_columns;";
print PSQL "DROP TABLE spatial_ref_sys;";
print "Restoring ascii dump $dumpascii\n";
#`psql -f $dumpascii $dbname`;
open(INPUT, "<$dumpascii") || die "Can't read $postgissql\n";
while(<INPUT>) { print PSQL; }
close(INPUT);
exit;


1;

