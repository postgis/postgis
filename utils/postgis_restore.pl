#/bin/sh
#
# This script is aimed at restoring postgis data
# from a dumpfile produced by pg_dump -Fc
#
# Basically it will restore all but things created by
# the given postgis.sql.
#
# A particular attention must be given to the spatial_ref_sys
# and geometry_columns tables which are created and populated
# from the dump, not the postgis.sql file. When the new installation
# is agains pgsql7.5+ and dump from pre7.5 this script should probably
# drop statistic fields from that table.... currently not done.
#
# Known issues:
#	- operators from the dump are never restored due to
#	  the impossibility (for current implementation) to
#	  detect wheter or not they are from postgis
#

exec perl $0 $@
	if (0);

(@ARGV == 3) || die "Usage: perl postgis_restore.pl <postgis.sql> <db> <dump>\nRestore a custom dump (pg_dump -Fc) of a postgis enabled database.\n";

$DEBUG=0;

my %aggs = {};
my %casts = ();
my %funcs = {};
my %types = {};

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
	#push (@ops, $line) if ($line =~ /^create operator.*\(/i);
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
			print "SKIPPING $id\n" if $DEBUG;
			next;
		}
		# This is an old postgis function which might
		# still be in a dump
		if ( $funcname eq 'unite_finalfunc' )
		{
			print "SKIPPING $id\n" if $DEBUG;
			next;
		}
		if ( $funcs{$id} )
		{
			print "SKIPPING PGIS $id\n" if $DEBUG;
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
		print "SKIPPING plpgsql\n" if $DEBUG;
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

	# Will skip all operators (there is no way to tell from a dump
	# list which types the operator works on
	elsif ($line =~ / OPERATOR /)
	{
		print "SKIPPING operator\n" if $DEBUG;
		next;
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
		#print "KEEPING CAST $id\n";
		#next;
	}
	print OUTPUT $line;
#	print "UNANDLED: $line"
}
close( INPUT );
close(OUTPUT);

#exit(1);
print "Creating db ($dbname)\n";
`createdb $dbname`;
print "Adding plpgsql\n";
`createlang plpgsql $dbname`;
print "Sourcing $postgissql\n";
`psql -f $postgissql $dbname`;
print "Restoring $dump\n";
`pg_restore -L $dumplist $dump | sed 's/^\\(SET search_path .*\\);/\\1, public;/' > $dumpascii`;
`psql -f $dumpascii $dbname`;
exit;


1;

