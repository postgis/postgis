# This script is aimed at restoring postgis data
# from a dumpfile produced by pg_dump -Fc
#
# Basically it will restore all but things created by
# the given postgis.sql.
#
# A particular attention is given to the spatial_ref_sys
# and geometry_columns tables which are created as from dump.
#
# Note that a postgresql-7.5 installation would require a shrink
# in the geomtry_columns table, due to internal gathering of
# statistics. The shrink is not performed so far (TODO).
#

(@ARGV == 3) || die "Usage: perl postgis_restore.pl <postgis.sql> <db> <dump>\nRestore a custom dump (pg_dump -Fc) of a postgis enabled database.\n";

my %aggs = {};
my %casts = ();
my %funcs = {};
my %types = {};
my @ops = ();

my $postgissql = $ARGV[0];
my $dbname = $ARGV[1];
my $dump = $ARGV[2];

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
		print "SQLFUNC: $id\n";
		next;
	}
	if ($line =~ /^create type +([^ ]+)/i)
	{
		my $type = $1;
		print "SQLTYPE $type\n";
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
		print "SQLAGG $id\n";
		$aggs{$id} = 1;
		next;
	}
	#if ($line =~ /^create cast \( ([^ ]*) AS ([^ ]*) \)/i)
	if ($line =~ /create cast \( *([^ ]*) *AS *([^ ]*) *\)/i)
	{
		my $id = lc($1).",".lc($2);
		print "SQLCAST $id\n";
		$casts{$id} = 1;
		next;
	}
	#push (@ops, $line) if ($line =~ /^create operator.*\(/i);
}
close( INPUT );
#exit;

print " ".@ops." operators [classes]\n";
print " ".@aggs." aggregates\n";
print " ".@casts." casts\n";

#
# Scan dump list
#
print "Scanning $dump list\n"; 
open( OUTPUT, ">".$dump.".list") || die "Can't write to ".$dump.".list\n";
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
			#print "SKIPPING $funcname($args)\n";
			next;
		}
		if ( $funcs{$id} )
		{
			print "SKIPPING PGIS $funcname($args) [".$funcs{$funcname."(".$args.")"}."]\n";
			next;
		}
		print "KEEPING FUNCTION: [$id]\n";
		#next;
	}
	if ($line =~ / AGGREGATE (.*)\((.*)\)/)
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
			#print "SKIPPING PGIS AGG $id\n";
			next;
		}
		print "KEEPING AGGREGATE [$id]\n";
		#next;
	}
	if ($line =~ / TYPE (.*) .*/)
	{
		my $type = $1;
		if ( $types{$type} )
		{
			#print "SKIPPING PGIS $funcname($args) [".$funcs{$funcname."(".$args.")"}."]\n";
			next;
		}
		#print "KEEPING TYPE [$type]\n";
		#next;
	}
	if ($line =~ / PROCEDURAL LANGUAGE plpgsql/)
	{
		#print "SKIPPING plpgsql\n";
		next;
	}

	# Will skip all operators (there is no way to tell from a dump
	# list which types the operator works on
	if ($line =~ / OPERATOR /)
	{
		#print "SKIPPING operator\n";
		next;
	}

	if ($line =~ / CAST *([^ ]*) *\( *([^ )]*) *\)/)
	{
		my $arg1 = lc($1);
		my $arg2 = lc($2);
		$arg1 =~ s/^public\.//;
		$arg2 =~ s/^public\.//;
		my $id = $arg2.",".$arg1;
		if ( $casts{$id} )
		{
			print "SKIPPING PGIS CAST $id\n";
			next;
		}
		print "KEEPING CAST $id\n";
		#next;
	}
	print OUTPUT $line;
	print "UNANDLED: $line"
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
print "Dropping spatial_ref_sys and geometry_columns\n";
`psql -c 'DROP TABLE spatial_ref_sys; DROP TABLE geometry_columns' $dbname`;
print "Restoring $dump\n";
$dumplist=$dump.".list";
`pg_restore -L $dumplist -d $dbname $dump`;
exit;


1;

