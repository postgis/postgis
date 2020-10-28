#!/usr/bin/env perl

#
# PostGIS - Spatial Types for PostgreSQL
# http://postgis.net
#
# Copyright (C) 2012-2014 Sandro Santilli <strk@kbt.io>
# Copyright (C) 2014-2015 Regina Obe <lr@pcorp.us>
# Copyright (C) 2012-2013 Paul Ramsey <pramsey@cleverelephant.ca>
#
# This is free software; you can redistribute and/or modify it under
# the terms of the GNU General Public Licence. See the COPYING file.
#

#$| = 1;
use File::Basename;
use File::Temp 'tempdir';
use Time::HiRes qw(time);
#use File::Which;
use File::Copy;
use File::Path;
use Cwd 'abs_path';
use Getopt::Long;
use strict;

##################################################################
# Add . to @INC if removed for CVE-2016-1238
##################################################################
BEGIN {
	push @INC, "." if(!grep /^\.$/, @INC);
}


##################################################################
# Usage ./run_test.pl <testname> [<testname>]
#
#  Create the spatial database 'postgis_reg' (or whatever $DB
#  is set to) if it doesn't already exist.
#
#  Run the <testname>.sql script
#  Check output against <testname>_expected
##################################################################

##################################################################
# Global configuration items
##################################################################

our $DB = $ENV{"POSTGIS_REGRESS_DB"} || "postgis_reg";
our $REGDIR = abs_path(dirname($0));
our $SHP2PGSQL = $REGDIR . "/../loader/shp2pgsql";
our $PGSQL2SHP = $REGDIR . "/../loader/pgsql2shp";
our $RASTER2PGSQL = $REGDIR . "/../raster/loader/raster2pgsql";
our $sysdiff = !system("diff --strip-trailing-cr $0 $0 2> /dev/null");

##################################################################
# Parse command line opts
##################################################################

my $OPT_CLEAN = 0;
my $OPT_NODROP = 0;
my $OPT_NOCREATE = 0;
my $OPT_UPGRADE = 0;
my $OPT_DUMPRESTORE = 0;
my $OPT_WITH_TOPO = 0;
my $OPT_WITH_RASTER = 0;
my $OPT_WITH_SFCGAL = 0;
my $OPT_EXPECT = 0;
my $OPT_EXTENSIONS = 0;
my $OPT_EXTVERSION = '';
my $OPT_UPGRADE_PATH = '';
our $OPT_UPGRADE_FROM = '';
my $OPT_UPGRADE_TO = '';
our $VERBOSE = 0;
our $OPT_SCHEMA = 'public';

GetOptions (
	'verbose' => \$VERBOSE,
	'clean' => \$OPT_CLEAN,
	'nodrop' => \$OPT_NODROP,
	'upgrade' => \$OPT_UPGRADE,
	'upgrade-path=s' => \$OPT_UPGRADE_PATH,
	'dumprestore' => \$OPT_DUMPRESTORE,
	'nocreate' => \$OPT_NOCREATE,
	'topology' => \$OPT_WITH_TOPO,
	'raster' => \$OPT_WITH_RASTER,
	'sfcgal' => \$OPT_WITH_SFCGAL,
	'expect' => \$OPT_EXPECT,
	'extensions' => \$OPT_EXTENSIONS,
	'schema=s' => \$OPT_SCHEMA
	);

if ( @ARGV < 1 )
{
	usage();
}

if ( $OPT_UPGRADE_PATH )
{
  if ( ! $OPT_EXTENSIONS )
  {
    die "--upgrade-path is only supported with --extension"
  }
  $OPT_UPGRADE = 1; # implied
  my @path = split ('--', $OPT_UPGRADE_PATH);
  $OPT_UPGRADE_FROM = $path[0]
    || die "Malformed upgrade path, <from>--<to> expected, $OPT_UPGRADE_PATH given";
  $OPT_UPGRADE_TO = $path[1]
    || die "Malformed upgrade path, <from>--<to> expected, $OPT_UPGRADE_PATH given";
  print "Upgrade path: ${OPT_UPGRADE_FROM} --> ${OPT_UPGRADE_TO}\n";
}

# Split-raster extension was introduced in PostGIS-3.0.0
sub has_split_raster_ext
{
  my $fullver = shift;
  # unpackaged is always current, so does have
  # split raster already.
  return 1 if $fullver =~ /^unpackaged/;
  my @ver = split(/\./, $fullver);
  return 0 if ( $ver[0] < 3 );
  return 1;
}

##################################################################
# Set the locale to "C" so error messages match
# Save original locale to set back
##################################################################

my $ORIG_LC_ALL = $ENV{"LC_ALL"};
my $ORIG_LANG = $ENV{"LANG"};
$ENV{"LC_ALL"} = "C";
$ENV{"LANG"} = "C";

# Add locale info to the psql options
# Add pg12 precision suppression
my $PGOPTIONS = $ENV{"PGOPTIONS"};
$PGOPTIONS .= " -c lc_messages=C";
$PGOPTIONS .= " -c client_min_messages=NOTICE";
$PGOPTIONS .= " -c extra_float_digits=0";
$ENV{"PGOPTIONS"} = $PGOPTIONS;

# Bring the path info in
my $PATH = $ENV{"PATH"}; # this is useless

# Calculate the regression directory locations
my $STAGED_INSTALL_DIR = $REGDIR . "/00-regress-install";
my $STAGED_SCRIPTS_DIR = $STAGED_INSTALL_DIR . "/share/contrib/postgis";

my $OBJ_COUNT_PRE = 0;
my $OBJ_COUNT_POST = 0;

##################################################################
# Check that we have the executables we need
##################################################################

print "PATH is $PATH\n";

foreach my $exec ( ($SHP2PGSQL, $PGSQL2SHP) )
{
	printf "Checking for %s ... ", basename($exec);
	if ( -x $exec )
	{
		print "found\n";
	}
	else
	{
		print "failed\n";
		die "Unable to find $exec executable.\n";
	}

}

if ( $OPT_WITH_RASTER )
{
	print "Checking for raster2pgsql ... ";
	if ( -x $RASTER2PGSQL )
	{
		print "found\n";
	}
	else
	{
		print "failed\n";
		die "Unable to find raster2pgsql executable.\n";
	}
}

##################################################################
# Set up the temporary directory
##################################################################

my $TMPDIR;
if ( $ENV{'PGIS_REG_TMPDIR'} )
{
	$TMPDIR = $ENV{'PGIS_REG_TMPDIR'};
}
elsif ( -d "/tmp/" && -w "/tmp/" )
{
	$TMPDIR = "/tmp/pgis_reg";
}
else
{
	$TMPDIR = tempdir( CLEANUP => 0 );
}

mkpath $TMPDIR; # make sure tmp dir exists

# Set log name
my $REGRESS_LOG = "${TMPDIR}/regress_log";

# Report
print "TMPDIR is $TMPDIR\n";

##################################################################
# Prepare the database
##################################################################

my @dblist = grep(/\b$DB\b/, split(/\n/, `psql -Xl`));
my $dbcount = @dblist;

if ( $dbcount == 0 )
{
	if ( $OPT_NOCREATE )
	{
		print "Database $DB does not exist.\n";
		print "Run without the --nocreate flag to create it.\n";
		exit(1);
	}
	else
	{
		create_spatial();
	}
}
else
{
	if ( $OPT_NOCREATE )
	{
		print "Using existing database $DB\n";
	}
	else
	{
		print STDERR "Database $DB already exists, dropping.\n";
		`dropdb $DB`;
		create_spatial();
	}
}

my $pgvernum = sql("SELECT current_setting('server_version_num')");
my $libver = sql("select postgis_lib_version()");
my $defextver = sql("select default_version from pg_available_extensions where name = 'postgis'");

if ( ! $libver )
{
	`dropdb $DB`;
	print "\nSomething went wrong (no PostGIS installed in $DB).\n";
	print "For details, check $REGRESS_LOG\n\n";
	exit(1);
}

sub scriptdir
{
	my ( $version, $systemwide ) = @_;
	my $scriptdir;
	if ( $systemwide or ( $version and $version ne $libver ) ) {
		my $pgis_majmin = $version;
		$pgis_majmin =~ s/^([1-9]*\.[0-9]*).*/\1/;
		$scriptdir = `pg_config --sharedir`;
		chop $scriptdir;
		$scriptdir .= "/contrib/postgis-" . $pgis_majmin;
	} else {
		$scriptdir = $STAGED_SCRIPTS_DIR;
	}
	#print "XXX: scriptdir: $scriptdir\n";
	return $scriptdir;
}

sub semver_lessthan
{
  my ($a,$b) = @_;
  my @acomp = split(/\./, $a);
  my @bcomp = split(/\./, $b);
	foreach my $ac (@acomp) {
		my $bc = shift(@bcomp);
		return 0 if not defined($bc); # $b has less components
		return 1 if ( $ac < $bc ); # $a is less than $b
		return 0 if ( $ac > $bc ); # $a is not less than $b
	}
	# $a is equal to $b so far, if $b has more elements,
	# it is bigger
	return @bcomp ? 1 : 0;
}

sub create_upgrade_test_objects
{
  # TODO: allow passing the "upgrade-init" script via commandline

  my $query = "create table upgrade_test(g1 geometry, g2 geography";
  $query .= ", r raster" if ( $OPT_WITH_RASTER );
  $query .= ")";
  my $ret = sql($query);
  unless ( $ret =~ /^CREATE/ ) {
    `dropdb $DB`;
    print "\nSomething went wrong creating upgrade_test table: $ret.\n";
    exit(1);
  }

  $query = "insert into upgrade_test(g1,g2) values ";
	$query .= "('POINT(0 0)', 'LINESTRING(0 0, 1 1)'), ";
	$query .= "('POINT(1 0)', 'LINESTRING(0 1, 1 1)');";
  my $ret = sql($query);
  unless ( $ret =~ /^INSERT/ ) {
    `dropdb $DB`;
    print "\nSomething went wrong populating upgrade_test table: $ret.\n";
    exit(1);
  }


  if ( $pgvernum >= 120000 ) {
    # We know upgrading with an st_union() based view
    # fails unless you're on PostgreSQL 12, so we don't
    # even try that.
    #
    # We could re-enable this test IF we fix the upgrade
    # in pre-12 versions. Refer to
    # https://trac.osgeo.org/postgis/ticket/4386
    #
    $query = "create view upgrade_view_test as ";
    $query .= "select st_union(g1) from upgrade_test;";
    my $ret = sql($query);
    unless ( $ret =~ /^CREATE/ ) {
      `dropdb $DB`;
      print "\nSomething went wrong creating upgrade_view_test view: $ret.\n";
      exit(1);
    }
  }

  if ( $OPT_WITH_RASTER )
  {
    $query = "UPDATE upgrade_test SET r = ";
    $query .= " ST_AddBand(ST_MakeEmptyRaster(10, 10, 1, 1, 2, 2, 0, 0,4326), 1, '8BSI'::text, -129, NULL);";
    $ret = sql($query);
    unless ( $ret =~ /^UPDATE/ ) {
      `dropdb $DB`;
      print "\nSomething went wrong setting raster into upgrade_test table: $ret.\n";
      exit(1);
    }

    $query = "set client_min_messages to error; select AddRasterConstraints('upgrade_test', 'r')";
    $ret = sql($query);
    unless ( $ret =~ /^t$/ ) {
      `dropdb $DB`;
      print "\nSomething went wrong adding raster constraints to upgrade_test: " . $ret . "\n";
      exit(1);
    }
  }

  if ( $OPT_WITH_TOPO )
  {
    $query = "select topology.createTopology('upgrade_test');";
    $ret = sql($query);
    unless ( $ret =~ /^[1-9][0-9]*$/ ) {
      `dropdb $DB`;
      print "\nSomething went wrong adding upgrade_test topology: " . $ret . "\n";
      exit(1);
    }
  }
}

sub drop_upgrade_test_objects
{
  # TODO: allow passing the "upgrade-cleanup" script via commandline

  my $ret = sql("drop view if exists upgrade_view_test;");
  unless ( $ret =~ /^DROP/ ) {
    `dropdb $DB`;
    print "\nSomething went wrong dropping spatial view: $ret.\n";
    exit(1);
  }

  my $ret = sql("drop table upgrade_test;");
  unless ( $ret =~ /^DROP/ ) {
    `dropdb $DB`;
    print "\nSomething went wrong dropping spatial tables: $ret.\n";
    exit(1);
  }

  if ( $OPT_WITH_TOPO )
  {
    my $query = "SELECT topology.DropTopology('upgrade_test');";
    $ret = sql($query);
    unless ( $ret =~ /^Topology 'upgrade_test' dropped$/ ) {
      `dropdb $DB`;
      print "\nSomething went wrong dropping upgrade_test topology: " . $ret . "\n";
      exit(1);
    }
  }
}


if ( $OPT_UPGRADE )
{
	print "  Upgrading from postgis $libver\n";

  create_upgrade_test_objects();

  if ( $OPT_EXTENSIONS )
  {
    upgrade_spatial_extensions();
  }
  else
  {
	  upgrade_spatial();
  }

  drop_upgrade_test_objects();

  # Update libver
  $libver = sql("select postgis_lib_version()");
}

if ( $OPT_DUMPRESTORE )
{
  dump_restore();
}


##################################################################
# Report PostGIS environment
##################################################################

my $geosver =  sql("select postgis_geos_version()");
my $projver = sql("select postgis_proj_version()");
my $libbuilddate = sql("select postgis_lib_build_date()");
my $pgsqlver = sql("select version()");
my $gdalver = sql("select postgis_gdal_version()") if $OPT_WITH_RASTER;
my $sfcgalver = sql("select postgis_sfcgal_version()") if $OPT_WITH_SFCGAL;
my $scriptver = sql("select postgis_scripts_installed()");
# postgis_lib_revision was introduced in 3.1.0
my $librev = semver_lessthan($scriptver, "3.1.0") ?
							sql("select postgis_svn_version()") :
							sql("select postgis_lib_revision()");
my $raster_scriptver = sql("select postgis_raster_scripts_installed()")
  if ( $OPT_WITH_RASTER );

print "$pgsqlver\n";
print "  Postgis $libver - (${librev}) - $libbuilddate\n";
print "  scripts ${scriptver}\n";
print "  raster scripts ${raster_scriptver}\n" if ( $OPT_WITH_RASTER );
print "  GEOS: $geosver\n" if $geosver;
print "  PROJ: $projver\n" if $projver;
print "  SFCGAL: $sfcgalver\n" if $sfcgalver;
print "  GDAL: $gdalver\n" if $gdalver;


##################################################################
# Set up some global variables
##################################################################
my $RUN = 0;
my $FAIL = 0;
my $SKIP = 0;
our $TEST = "";

##################################################################
# Run the tests
##################################################################

print "\nRunning tests\n\n";

foreach $TEST (@ARGV)
{
	my $TEST_OBJ_COUNT_PRE;
	my $TEST_OBJ_COUNT_POST;
	my $TEST_START_TIME;

	# catch a common mistake (strip trailing .sql)
	$TEST =~ s/.sql$//;

	start_test($TEST);
	$TEST_OBJ_COUNT_PRE = count_postgis_objects();

	# Check for a "-pre.pl" file in case there are setup commands
	unless ( eval_file("${TEST}-pre.pl") )
	{
		chop($@);
		fail("Failed evaluating ${TEST}-pre.pl: $@");
		next;
	}

	# Check for a "-pre.sql" file in case there is setup SQL needed before
	# the test can be run.
	if ( -r "${TEST}-pre.sql" )
	{
		run_simple_sql("${TEST}-pre.sql");
		show_progress();
	}

	# Check .dbf *before* .sql as loader test could
	# create the .sql
	# Check for .dbf not just .shp since the loader can load
	# .dbf files without a .shp.
	$TEST_START_TIME = time;
	if ( -r "${TEST}.dbf" )
	{
		pass("in ".int(1000*(time-$TEST_START_TIME))." ms") if ( run_loader_test() );
	}
	elsif ( -r "${TEST}.tif" )
	{
		my $rv = run_raster_loader_test();
		pass("in ".int(1000*(time-$TEST_START_TIME))." ms") if $rv;
	}
	elsif ( -r "${TEST}.sql" )
	{
		my $rv = run_simple_test("${TEST}.sql", "${TEST}_expected");
		pass("in ".int(1000*(time-$TEST_START_TIME))." ms") if $rv;
	}
	elsif ( -r "${TEST}.dmp" )
	{
		pass("in ".int(1000*(time-$TEST_START_TIME))." ms") if run_dumper_test();
	}
	else
	{
		print " skipped (can't read any ${TEST}.{sql,dbf,tif,dmp})\n";
		$SKIP++;
		# Even though we skipped this test, we will still run the cleanup
		# scripts
	}

	if ( -r "${TEST}-post.sql" )
	{
		my $rv = run_simple_sql("${TEST}-post.sql");
		if ( ! $rv )
		{
			print " ... but cleanup sql failed!";
		}
	}

	# Check for a "-post.pl" file in case there are teardown commands
	eval_file("${TEST}-post.pl");

	$TEST_OBJ_COUNT_POST = count_postgis_objects();

	if ( $TEST_OBJ_COUNT_POST != $TEST_OBJ_COUNT_PRE )
	{
		fail("PostGIS object count pre-test ($TEST_OBJ_COUNT_POST) != post-test ($TEST_OBJ_COUNT_PRE)");
	}

}


###################################################################
# Uninstall postgis (serves as an uninstall test)
##################################################################

# We only test uninstall if we've been asked to drop
# and we did create
# and nobody requested raster or topology
# (until they have an uninstall script themself)

if ( (! $OPT_NODROP) && $OBJ_COUNT_PRE > 0 )
{
	uninstall_spatial();
}

##################################################################
# Summary report
##################################################################

print "\nRun tests: $RUN\n";
print "Failed: $FAIL\n";

if ( $OPT_CLEAN )
{
	rmtree($TMPDIR);
}

if ( ! ($OPT_NODROP || $OPT_NOCREATE) )
{
	system("dropdb $DB");
}
else
{
	print "Drop database ${DB} manually\n";
}

# Set the locale back to the original
$ENV{"LC_ALL"} = $ORIG_LC_ALL;
$ENV{"LANG"} = $ORIG_LANG;

exit($FAIL);



##################################################################
# Utility functions
#

sub usage
{
	die qq{
Usage: $0 [<options>] <testname> [<testname>]
Options:
  -v, --verbose   be verbose about failures
  --nocreate      do not create the regression database on start
  --upgrade       source the upgrade scripts on start
  --upgrade-path  upgrade path, format <from>--<to>.
                  <from> can be specified as "unpackaged<version>"
                         to specify a script version to start from.
                  <to> can be specified as ":auto" to request
                       upgrades to default version, and be appended
                       a question mark (ie: ":auto!" or "3.0.0!") to
                       request upgrade via postgis_extensions_upgrade()
                       if available.
  --dumprestore   dump and restore spatially-enabled db before running tests
  --nodrop        do not drop the regression database on exit
  --schema        where to install/find PostGIS (relocatable) PostGIS
                  (defaults to "public")
  --raster        load also raster extension
  --topology      load also topology extension
  --sfcgal        use also sfcgal backend
  --clean         cleanup test logs on exit
  --expect        save obtained output as expected
  --extension     load using extensions
};

}

# start_test <name>
sub start_test
{
    my $test = shift;
    print " $test ";
	$RUN++;
    show_progress();
}

# Print a entry
sub echo_inline
{
	my $msg = shift;
	print $msg;
}

# Print a single dot
sub show_progress
{
	print ".";
}

# pass <msg>
sub pass
{
    my $msg = shift;
    printf(" ok %s\n", $msg);
}

# fail <msg> <log>
sub fail
{
	my $msg = shift;
	my $log = shift;

	if ( ! $log )
	{
		printf(" failed (%s)\n", $msg);
	}
	elsif ( $VERBOSE == 1 )
	{
		printf(" failed (%s: %s)\n", $msg, $log);
		print "-----------------------------------------------------------------------------\n";
		open(LOG, "$log") or die "Cannot open log file $log\n";
		print while(<LOG>);
		close(LOG);
		print "-----------------------------------------------------------------------------\n";
	}
	else
	{
		printf(" failed (%s: %s)\n", $msg, $log);
	}

	$FAIL++;
}



##################################################################
# run_simple_sql
#   Run an sql script and hide results unless it fails.
#   SQL input file name is $1
##################################################################
sub run_simple_sql
{
	my $sql = shift;

	if ( ! -r $sql )
	{
		fail("can't read $sql");
		return 0;
	}

	# Dump output to a temp file.
	my $tmpfile = sprintf("%s/test_%s_tmp", $TMPDIR, $RUN);
	my $cmd = "psql -v \"VERBOSITY=terse\" -tXAq $DB < $sql > $tmpfile 2>&1";
	#print($cmd);
	my $rv = system($cmd);
	# Check if psql errored out.
	if ( $rv != 0 )
	{
		fail("Unable to run sql script $sql", $tmpfile);
		return 0;
	}

	# Check for ERROR lines
	open FILE, "$tmpfile";
	my @lines = <FILE>;
	close FILE;
	my @errors = grep(/^ERROR/, @lines);

	if ( @errors > 0 )
	{
		fail("Errors while running sql script $sql", $tmpfile);
		return 0;
	}

	unlink $tmpfile;
	return 1;
}

sub drop_table
{
	my $tblname = shift;
	my $cmd = "psql -tXAq -d $DB -c \"DROP TABLE IF EXISTS $tblname\" >> $REGRESS_LOG 2>&1";
	my $rv = system($cmd);
	die "Could not run: $cmd\n" if $rv;
}

sub sql
{
	my $sql = shift;
	my $result = `psql -tXA -d $DB -c 'SET search_path TO public,$OPT_SCHEMA' -c "$sql" | sed '/^SET\$/d'`;
	$result =~ s/[\n\r]*$//;
	$result;
}

sub eval_file
{
    my $file = shift;
    my $pl;
    if ( -r $file )
    {
				do $file or return 0;
				#system($^X, $file) == 0 or return 0;
    }
		1;
}

##################################################################
# run_simple_test
#   Run an sql script and compare results with the given expected output
#   SQL input is ${TEST}.sql, expected output is {$TEST}_expected
##################################################################
sub run_simple_test
{
	my $sql = shift;
	my $expected = shift;
	my $msg = shift;

	if ( ! -r "$sql" )
	{
		fail("can't read $sql");
		return 0;
	}

	if ( ! $OPT_EXPECT )
	{
		if ( ! -r "$expected" )
		{
			fail("can't read $expected");
			return 0;
		}
	}

	show_progress();

	my $outfile = sprintf("%s/test_%s_out", $TMPDIR, $RUN);
	my $betmpdir = sprintf("%s/pgis_reg_tmp/", $TMPDIR);
	my $tmpfile = sprintf("%s/test_%s_tmp", $betmpdir, $RUN);
	my $diffile = sprintf("%s/test_%s_diff", $TMPDIR, $RUN);

	mkpath($betmpdir);
	chmod 0777, $betmpdir;

	my $scriptdir = scriptdir($libver, $OPT_EXTENSIONS);

	my ($sqlfile,$sqldir) = fileparse($sql);
	my $cmd = "cd $sqldir; psql -v \"VERBOSITY=terse\""
          . " -v \"tmpfile='$tmpfile'\""
          . " -v \"scriptdir=$scriptdir\""
          . " -v \"regdir=$REGDIR\""
          . " -v \"schema=$OPT_SCHEMA.\""
          . " -c \"SET search_path TO public,$OPT_SCHEMA,topology\""
          . " -tXAq -f $sqlfile $DB > $outfile 2>&1";
	my $rv = system($cmd);

	# Check for ERROR lines
	open(FILE, "$outfile");
	my @lines = <FILE>;
	close(FILE);

	# Strip the lines we don't care about
	@lines = grep(!/^\$/, @lines);
	@lines = grep(!/^(INSERT|DELETE|UPDATE|SELECT|COPY|DO)/, @lines);
	@lines = grep(!/^(CONTEXT|RESET|ANALYZE)/, @lines);
	@lines = grep(!/^(DROP|CREATE|ALTER|VACUUM)/, @lines);
	@lines = grep(!/^(LOG|SET|TRUNCATE|DISCARD)/, @lines);
	@lines = grep(!/^LINE \d/, @lines);
	@lines = grep(!/^\s+$/, @lines);

	# Morph values into expected forms
	for ( my $i = 0; $i < @lines; $i++ )
	{
		$lines[$i] =~ s/Infinity/inf/g;
		$lines[$i] =~ s/Inf/inf/g;
		$lines[$i] =~ s/1\.#INF/inf/g;
		$lines[$i] =~ s/[eE]([+-])0+(\d+)/e$1$2/g;
		$lines[$i] =~ s/Self-intersection .*/Self-intersection/;
		$lines[$i] =~ s/^ROLLBACK/COMMIT/;
		$lines[$i] =~ s/^psql.*(NOTICE|WARNING|ERROR):/\1:/g;
	}

	# Write out output file
	open(FILE, ">$outfile");
	foreach my $l (@lines)
	{
		print FILE $l;
	}
	close(FILE);

	# Clean up interim stuff
	#remove_tree($betmpdir);

	if ( $OPT_EXPECT )
	{
		print " (expected)";
		copy($outfile, $expected);
	}
	else
	{
		my $diff = diff($expected, $outfile);
		if ( $diff )
		{
			open(FILE, ">$diffile");
			print FILE $diff;
			close(FILE);
			fail("${msg}diff expected obtained", $diffile);
			return 0;
		}
		else
		{
			unlink $outfile;
			return 1;
		}
	}

	return 1;
}

##################################################################
# This runs the loader once and checks the output of it.
# It will NOT run if neither the expected SQL nor the expected
# select results file exists, unless you pass true for the final
# parameter.
#
# $1 - Description of this run of the loader, used for error messages.
# $2 - Table name to load into.
# $3 - The name of the file containing what the
#      SQL generated by shp2pgsql should look like.
# $4 - The name of the file containing the expected results of
#      SELECT geom FROM _tblname should look like.
# $5 - Command line options for shp2pgsql.
# $6 - If you pass true, this will run the loader even if neither
#      of the expected results files exists (though of course
#      the results won't be compared with anything).
##################################################################
sub run_loader_and_check_output
{
	my $description = shift;
	my $tblname = shift;
	my $expected_sql_file = shift;
	my $expected_select_results_file = shift;
	my $loader_options = shift;
	my $run_always = shift;

	my ( $cmd, $rv );
	my $outfile = "${TMPDIR}/loader.out";
	my $errfile = "${TMPDIR}/loader.err";

	# ON_ERROR_STOP is used by psql to return non-0 on an error
	my $psql_opts = " --no-psqlrc --variable ON_ERROR_STOP=true";

	if ( $run_always || -r $expected_sql_file || -r $expected_select_results_file )
	{
		show_progress();
		# Produce the output SQL file.
		$cmd = "$SHP2PGSQL $loader_options -g the_geom ${TEST}.shp $tblname > $outfile 2> $errfile";
		$rv = system($cmd);

		if ( $rv )
		{
			fail(" $description: running $cmd", "$errfile");
			return 0;
		}

		# Compare the output SQL file with the expected if there is one.
		if ( -r $expected_sql_file )
		{
			show_progress();
			my $diff = diff($expected_sql_file, $outfile);
			if ( $diff )
			{
				fail(" $description: actual SQL does not match expected.", "$outfile");
				return 0;
			}
		}

		# Run the loader SQL script.
		show_progress();
		$cmd = "psql $psql_opts -f $outfile $DB > $errfile 2>&1";
		$rv = system($cmd);
		if ( $rv )
		{
			fail(" $description: running shp2pgsql output","$errfile");
			return 0;
		}

		# Run the select script (if there is one)
		if ( -r "${TEST}.select.sql" )
		{
			$rv = run_simple_test("${TEST}.select.sql",$expected_select_results_file, $description);
			return 0 if ( ! $rv );
		}
	}
	return 1;
}

##################################################################
# This runs the dumper once and checks the output of it.
# It will NOT run if the expected shp file does not exist, unless
# you pass true for the final parameter.
#
# $1 - Description of this run of the dumper, used for error messages.
# $2 - Table name to dump from.
# $3 - "Expected" .shp file to compare with.
# $4 - If you pass true, this will run the loader even if neither
#      of the expected results files exists (though of course
#      the results won't be compared with anything).
##################################################################
sub run_dumper_and_check_output
{
	my $description = shift;
	my $tblname = shift;
	my $expected_shp_file = shift;
	my $run_always = shift;

	my ($cmd, $rv);
	my $errfile = "${TMPDIR}/dumper.err";

	if ( $run_always || -r $expected_shp_file )
	{
		show_progress();
		$cmd = "${PGSQL2SHP} -f ${TMPDIR}/dumper $DB $tblname > $errfile 2>&1";
		$rv = system($cmd);

		if ( $rv )
		{
			fail("$description: dumping loaded table", $errfile);
			return 0;
		}

		# Compare with expected output if there is any.

		if ( -r $expected_shp_file )
		{
			show_progress();

			my $diff = diff($expected_shp_file,  "$TMPDIR/dumper.shp");
			if ( $diff )
			{
#				ls -lL "${TMPDIR}"/dumper.shp "$_expected_shp_file" > "${TMPDIR}"/dumper.diff
				fail("$description: dumping loaded table", "${TMPDIR}/dumper.diff");
				return 0;
			}
		}
	}
	return 1;
}


##################################################################
# This runs the loader once and checks the output of it.
# It will NOT run if neither the expected SQL nor the expected
# select results file exists, unless you pass true for the final
# parameter.
#
# $1 - Description of this run of the loader, used for error messages.
# $2 - Table name to load into.
# $3 - The name of the file containing what the
#      SQL generated by shp2pgsql should look like.
# $4 - The name of the file containing the expected results of
#      SELECT rast FROM _tblname should look like.
# $5 - Command line options for raster2pgsql.
# $6 - If you pass true, this will run the loader even if neither
#      of the expected results files exists (though of course
#      the results won't be compared with anything).
##################################################################
sub run_raster_loader_and_check_output
{
	my $description = shift;
	my $tblname = shift;
	my $expected_sql_file = shift;
	my $expected_select_results_file = shift;
	my $loader_options = shift;
	my $run_always = shift;

	# ON_ERROR_STOP is used by psql to return non-0 on an error
	my $psql_opts="--no-psqlrc --variable ON_ERROR_STOP=true";

	my ($cmd, $rv);
	my $outfile = "${TMPDIR}/loader.out";
	my $errfile = "${TMPDIR}/loader.err";

	if ( $run_always || -r $expected_sql_file || -r $expected_select_results_file )
	{
		show_progress();

		# Produce the output SQL file.
		$cmd = "$RASTER2PGSQL $loader_options ${TEST}.tif $tblname > $outfile 2> $errfile";
		$rv = system($cmd);

		if ( $rv )
		{
		    fail("$description: running raster2pgsql", $errfile);
		    return 0;
	    }

	    if ( -r $expected_sql_file )
	    {
	        show_progress();
			my $diff = diff($expected_sql_file, $outfile);
			if ( $diff )
			{
				fail(" $description: actual SQL does not match expected.", "$outfile");
				return 0;
			}

        }

		# Run the loader SQL script.
		show_progress();
		$cmd = "psql $psql_opts -f $outfile $DB > $errfile 2>&1";
    	$rv = system($cmd);
    	if ( $rv )
    	{
    		fail(" $description: running raster2pgsql output","$errfile");
    		return 0;
    	}

    	# Run the select script (if there is one)
    	if ( -r "${TEST}.select.sql" )
    	{
    		$rv = run_simple_test("${TEST}.select.sql",$expected_select_results_file, $description);
    		return 0 if ( ! $rv );
    	}
	}

    return 1;
}



##################################################################
#  run_loader_test
#
#  Load a shapefile with different methods, create a 'select *' SQL
#  test and run simple test with provided expected output.
#
#  SHP input is ${TEST}.shp, expected output is {$TEST}.expected
##################################################################
sub run_loader_test
{
	# See if there is a custom command-line options file
	my $opts_file = "${TEST}.opts";
	my $custom_opts="";

	if ( -r $opts_file )
	{
		open(FILE, $opts_file);
		my @opts;
		while (<FILE>) {
			next if /^\s*#/;
			chop;
			s/{regdir}/$REGDIR/;
			push @opts, $_;
		}
		close(FILE);
		$custom_opts = join(" ", @opts);
	}

	my $tblname="loadedshp";

	# If we have some expected files to compare with, run in wkt mode.
	if ( ! run_loader_and_check_output("wkt test", $tblname, "${TEST}-w.sql.expected", "${TEST}-w.select.expected", "-w $custom_opts") )
	{
		return 0;
	}
	drop_table($tblname);

	# If we have some expected files to compare with, run in geography mode.
	if ( ! run_loader_and_check_output("geog test", $tblname, "${TEST}-G.sql.expected", "${TEST}-G.select.expected", "-G $custom_opts") )
	{
		return 0;
	}
	# If we have some expected files to compare with, run the dumper and compare shape files.
	if ( ! run_dumper_and_check_output("dumper geog test", $tblname, "${TEST}-G.shp.expected") )
	{
		return 0;
	}
	drop_table($tblname);

	# Always run in wkb ("normal") mode, even if there are no expected files to compare with.
	if( ! run_loader_and_check_output("wkb test", $tblname, "${TEST}.sql.expected", "${TEST}.select.expected", "$custom_opts", "true") )
	{
		return 0;
	}
	# If we have some expected files to compare with, run the dumper and compare shape files.
	if( ! run_dumper_and_check_output("dumper wkb test", $tblname, "${TEST}.shp.expected") )
	{
		return 0;
	}
	drop_table($tblname);

	# Some custom parameters can be incompatible with -D.
	if ( $custom_opts )
	{
		# If we have some expected files to compare with, run in wkt dump mode.
		if ( ! run_loader_and_check_output("wkt dump test", $tblname, "${TEST}-wD.sql.expected") )
		{
			return 0;
		}
		drop_table($tblname);

		# If we have some expected files to compare with, run in wkt dump mode.
		if ( ! run_loader_and_check_output("geog dump test", $tblname, "${TEST}-GD.sql.expected") )
		{
			return 0;
		}
		drop_table($tblname);

		# If we have some expected files to compare with, run in wkb dump mode.
		if ( ! run_loader_and_check_output("wkb dump test", $tblname, "${TEST}-D.sql.expected") )
		{
			return 0;
		}
		drop_table($tblname);
	}

	return 1;
}

##################################################################
#  run_dumper_test
#
#  Run dumper and compare output with various expectances
#  test and run simple test with provided expected output.
#
# input is ${TEST}.dmp, where first line is considered to be the
# [table|query] argument for pgsql2shp and all the next lines,
# if any are options.
#
##################################################################
sub run_dumper_test
{
  my $dump_file  = "${TEST}.dmp";

  # ON_ERROR_STOP is used by psql to return non-0 on an error
  my $psql_opts="--no-psqlrc --variable ON_ERROR_STOP=true";

  my $shpfile = "${TMPDIR}/dumper-" . basename(${TEST}) . "-shp";
  my $outfile = "${TMPDIR}/dumper-" . basename(${TEST}) . ".out";
  my $errfile = "${TMPDIR}/dumper-" . basename(${TEST}) . ".err";

  # Produce the output SHP file.
  open DUMPFILE, "$dump_file" or die "Cannot open dump file $dump_file\n";
  my @dumplines = <DUMPFILE>;
  close DUMPFILE;
  chomp(@dumplines);
  my $dumpstring = shift @dumplines;
  @dumplines = map { local $_ = $_; s/{regdir}/$REGDIR/; $_ } @dumplines;
  my @cmd = ("${PGSQL2SHP}", "-f", ${shpfile});
  push @cmd, @dumplines;
  push @cmd, ${DB};
  push @cmd, $dumpstring;
  #print "CMD: " . join (' ', @cmd) . "\n";
  open my $stdout_save, '>&', *STDOUT or die "Cannot dup stdout\n";
  open my $stderr_save, '>&', *STDERR or die "Cannot dup stdout\n";
  open STDOUT, ">${outfile}" or die "Cannot write to ${outfile}\n";
  open STDERR, ">${errfile}" or die "Cannot write to ${errfile}\n";
  my $rv = system(@cmd);
  open STDERR, '>&', $stderr_save;
  open STDOUT, '>&', $stdout_save;
  show_progress();

  if ( $rv )
  {
    fail("dumping", "$errfile");
    return 0;
  }

  my $numtests = 0;
  foreach my $ext ("shp","prj","dbf","shx") {
    my $obtained = ${shpfile}.".".$ext;
    my $expected = ${TEST}."_expected.".$ext;
    if ( $OPT_EXPECT )
    {
      copy($obtained, $expected);
    }
    elsif ( -r ${expected} ) {
      show_progress();
      $numtests++;
      my $diff = diff($expected,  $obtained);
      if ( $diff )
      {
        my $diffile = sprintf("%s/dumper_test_%s_diff", $TMPDIR, $ext);
        open(FILE, ">$diffile");
        print FILE $diff;
        close(FILE);
        fail("diff expected obtained", $diffile);
        return 0;
      }
    }
  }

  #show_progress();

  if ( $OPT_EXPECT ) {
    print " (expected)";
  }
  elsif ( ! $numtests ) {
    fail("no expectances!");
    return 0;
  }

	return 1;
}


##################################################################
#  run_raster_loader_test
##################################################################
sub run_raster_loader_test
{
	# See if there is a custom command-line options file
	my $opts_file = "${TEST}.opts";
	my $custom_opts="";

	if ( -r $opts_file )
	{
		open(FILE, $opts_file);
		my @opts = <FILE>;
		close(FILE);
		@opts = grep(!/^\s*#/, @opts);
		map(s/\n//, @opts);
		$custom_opts = join(" ", @opts);
	}

	my $tblname="loadedrast";

	# If we have some expected files to compare with, run in geography mode.
	if ( ! run_raster_loader_and_check_output("test", $tblname, "${TEST}.sql.expected", "${TEST}.select.expected", $custom_opts, "true") )
	{
		return 0;
	}

	drop_table($tblname);

	return 1;
}


##################################################################
# Count database objects
##################################################################
sub count_db_objects
{
	my $count = sql("WITH counts as (
		select count(*) from pg_type union all
		select count(*) from pg_proc union all
		select count(*) from pg_cast union all
		select count(*) from pg_aggregate union all
		select count(*) from pg_operator union all
		select count(*) from pg_opclass union all
		select count(*) from pg_namespace
			where nspname NOT LIKE 'pg_%'
			  and nspname != '${OPT_SCHEMA}'
		union all
		select count(*) from pg_opfamily
		)
		select sum(count) from counts");

 	return $count;
}


##################################################################
# Count postgis objects
##################################################################
sub count_postgis_objects
{
	my $count = sql("WITH counts as (
		select count(*) from spatial_ref_sys
		)
		select sum(count) from counts");

 	return $count;
}



##################################################################
# Create the spatial database
##################################################################
sub create_db
{
	my $createcmd = "createdb --encoding=UTF-8 --template=template0 --lc-collate=C $DB > $REGRESS_LOG";
	return system($createcmd);
}

sub create_spatial
{
	my ($cmd, $rv);
	print "Creating database '$DB' \n";

  	$rv = create_db();

	# Count database objects before installing anything
	$OBJ_COUNT_PRE = count_db_objects();

	if ( $OPT_EXTENSIONS )
	{
		prepare_spatial_extensions();
	}
	else
	{
		prepare_spatial();
	}
}


sub load_sql_file
{
	my $file = shift;
	my $strict = shift;

	if ( $strict && ! -e $file )
	{
		die "Unable to find $file\n";
	}

	if ( -e $file )
	{
		# ON_ERROR_STOP is used by psql to return non-0 on an error
		my $psql_opts = "--no-psqlrc --variable ON_ERROR_STOP=true";
		my $cmd = "psql $psql_opts -c 'CREATE SCHEMA IF NOT EXISTS $OPT_SCHEMA' ";
		$cmd .= "-c 'SET search_path TO $OPT_SCHEMA,topology'";
		$cmd .= " -Xf $file $DB >> $REGRESS_LOG 2>&1";
		#print "  $file\n" if $VERBOSE;
		my $rv = system($cmd);
		if ( $rv )
		{
		  fail "Error encountered loading $file", $REGRESS_LOG;
		  exit 1;
		}
	}
	return 1;
}

# Prepare the database for spatial operations (extension method)
sub prepare_spatial_extensions
{
	# ON_ERROR_STOP is used by psql to return non-0 on an error
	my $psql_opts = "--no-psqlrc --variable ON_ERROR_STOP=true";

	my $sql = "CREATE SCHEMA IF NOT EXISTS ${OPT_SCHEMA}";
	my $cmd = "psql $psql_opts -c \"". $sql . "\" $DB >> $REGRESS_LOG 2>&1";
	my $rv = system($cmd);
	if ( $rv ) {
	  fail "Error encountered creating target schema ${OPT_SCHEMA}", $REGRESS_LOG;
	  die;
	}

	my $sql = "CREATE EXTENSION postgis";

	if ( $OPT_UPGRADE_FROM ) {
		if ( $OPT_UPGRADE_FROM =~ /^unpackaged(.*)/ ) {
			prepare_spatial($1);
			return;
		}
		$sql .= " VERSION '" . $OPT_UPGRADE_FROM . "'";
	}

	$sql .= " SCHEMA " . $OPT_SCHEMA;

	print "Preparing db '${DB}' using: ${sql}\n";

	my $cmd = "psql $psql_opts -c \"". $sql . "\" $DB >> $REGRESS_LOG 2>&1";
	my $rv = system($cmd);

  if ( $rv ) {
  	fail "Error encountered creating EXTENSION POSTGIS", $REGRESS_LOG;
  	die;
	}

	if ( $OPT_WITH_TOPO )
	{
		my $sql = "CREATE EXTENSION postgis_topology";
		if ( $OPT_UPGRADE_FROM ) {
			$sql .= " VERSION '" . $OPT_UPGRADE_FROM . "'";
		}

		print "Preparing db '${DB}' using: ${sql}\n";

 		$cmd = "psql $psql_opts -c \"" . $sql . "\" $DB >> $REGRESS_LOG 2>&1";
		$rv = system($cmd);
  	if ( $rv ) {
  		fail "Error encountered creating EXTENSION POSTGIS_TOPOLOGY", $REGRESS_LOG;
  		die;
		}
 	}

	if ( $OPT_WITH_RASTER && (
		# NOTE: this code is assuming that the default version
		# (!$OPT_UPGRADE_FROM) has split raster extension
		! $OPT_UPGRADE_FROM ||
		has_split_raster_ext($OPT_UPGRADE_FROM)
	) )
	{
		my $sql = "CREATE EXTENSION postgis_raster";
		if ( $OPT_UPGRADE_FROM ) {
			$sql .= " VERSION '" . $OPT_UPGRADE_FROM . "'";
		}

		$sql .= " SCHEMA " . $OPT_SCHEMA;

		print "Preparing db '${DB}' using: ${sql}\n";

 		$cmd = "psql $psql_opts -c \"" . $sql . "\" $DB >> $REGRESS_LOG 2>&1";
		$rv = system($cmd);
		if ( $rv ) {
			fail "Error encountered creating EXTENSION POSTGIS_RASTER", $REGRESS_LOG;
			die;
		}
 	}

	if ( $OPT_WITH_SFCGAL )
	{
		{
			my $sql = "CREATE EXTENSION postgis_sfcgal";
			if ( $OPT_UPGRADE_FROM ) {
				if ( semver_lessthan($OPT_UPGRADE_FROM, "2.2.0") )
				{
					print "NOTICE: skipping SFCGAL extension create "
							. "as not available in version '$OPT_UPGRADE_FROM'\n";
					last;
				}
				$sql .= " VERSION '" . $OPT_UPGRADE_FROM . "'";
			}

			$sql .= " SCHEMA " . $OPT_SCHEMA;

			print "Preparing db '${DB}' using: ${sql}\n";

			$cmd = "psql $psql_opts -c \"" . $sql . "\" $DB >> $REGRESS_LOG 2>&1";
			$rv = system($cmd);
			if ( $rv ) {
				fail "Error encountered creating EXTENSION POSTGIS_SFCGAL", $REGRESS_LOG;
				die;
			}
		}
	}

 	return 1;
}

# Prepare the database for spatial operations (old method)
sub prepare_spatial
{
	my $version = shift;
	my $scriptdir = scriptdir($version);
	print "Loading unpackaged components from $scriptdir\n";

	print "Loading PostGIS into '${DB}' \n";

	# Load postgis.sql into the database
	load_sql_file("${scriptdir}/postgis.sql", 1);
	load_sql_file("${scriptdir}/postgis_comments.sql", 0);
	load_sql_file("${scriptdir}/spatial_ref_sys.sql", 0);

	if ( $OPT_WITH_TOPO )
	{
		print "Loading Topology into '${DB}'\n";
		load_sql_file("${scriptdir}/topology.sql", 1);
		load_sql_file("${scriptdir}/topology_comments.sql", 0);
	}

	if ( $OPT_WITH_RASTER )
	{
		print "Loading Raster into '${DB}'\n";
		load_sql_file("${scriptdir}/rtpostgis.sql", 1);
		load_sql_file("${scriptdir}/raster_comments.sql", 0);
	}

	if ( $OPT_WITH_SFCGAL )
	{
		print "Loading SFCGAL into '${DB}'\n";
		load_sql_file("${scriptdir}/sfcgal.sql", 1);
		load_sql_file("${scriptdir}/sfcgal_comments.sql", 0);
	}

	return 1;
}

sub package_extension_sql
{
	my ($extname, $extver) = @_;
	my $sql;

	if ( $pgvernum lt 130000 ) {
		$sql = "CREATE EXTENSION ${extname} VERSION '${extver}' FROM unpackaged;";
	} else {
		$sql = "CREATE EXTENSION ${extname} VERSION unpackaged;";
		$sql .= "ALTER EXTENSION ${extname} UPDATE TO '${extver}'";
	}
	return $sql;
}

# Upgrade an existing database (soft upgrade)
sub upgrade_spatial
{
    print "Upgrading PostGIS in '${DB}' \n" ;

    my $script = `ls ${STAGED_SCRIPTS_DIR}/postgis_upgrade.sql`;
    chomp($script);

    if ( -e $script )
    {
        print "Upgrading core\n";
        load_sql_file($script);
    }
    else
    {
        die "$script not found\n";
    }

    if ( $OPT_WITH_TOPO )
    {
        my $script = `ls ${STAGED_SCRIPTS_DIR}/topology_upgrade.sql`;
        chomp($script);
        if ( -e $script )
        {
            print "Upgrading topology\n";
            load_sql_file($script);
        }
        else
        {
            die "$script not found\n";
        }
    }

    if ( $OPT_WITH_RASTER )
    {
        my $script = `ls ${STAGED_SCRIPTS_DIR}/rtpostgis_upgrade.sql`;
        chomp($script);
        if ( -e $script )
        {
            print "Upgrading raster\n";
            load_sql_file($script);
        }
        else
        {
            die "$script not found\n";
        }
    }

    if ( $OPT_WITH_SFCGAL )
    {
        my $script = `ls ${STAGED_SCRIPTS_DIR}/sfcgal_upgrade.sql`;
        chomp($script);
        if ( -e $script )
        {
            print "Upgrading sfcgal\n";
            load_sql_file($script);
        }
        else
        {
            die "$script not found\n";
        }
    }

    return 1;
}

# Upgrade an existing database (soft upgrade, extension method)
sub upgrade_spatial_extensions
{
    # ON_ERROR_STOP is used by psql to return non-0 on an error
    my $psql_opts = "--no-psqlrc --variable ON_ERROR_STOP=true";
    my $sql;
    my $upgrade_via_function = 0;

    if ( $OPT_UPGRADE_TO =~ /!$/ )
    {
      $OPT_UPGRADE_TO =~ s/!$//;
      my $from = $OPT_UPGRADE_FROM;
      $from =~ s/^unpackaged//;
      if ( ! $from || ! semver_lessthan($from, "3.0.0") )
      {
        $upgrade_via_function = 1;
      }
      else
      {
        print "WARNING: postgis_extensions_upgrade()".
              " not available or functional in version $from.".
              " We'll use manual upgrade.\n";
      }
    }

    if ( $OPT_UPGRADE_TO =~ /^:auto/ )
    {
      $OPT_UPGRADE_TO = $defextver;
    }

    my $nextver = $OPT_UPGRADE_TO ? "${OPT_UPGRADE_TO}" : "${libver}next";

    if ( $upgrade_via_function )
    {
      $sql = "SELECT postgis_extensions_upgrade()";
    }
    elsif ( $OPT_UPGRADE_FROM =~ /^unpackaged/ )
    {
			$sql = package_extension_sql('postgis', ${nextver});
    }
    else
    {
      $sql = "ALTER EXTENSION postgis UPDATE TO '${nextver}'";
    }

    print "Upgrading PostGIS in '${DB}' using: ${sql}\n" ;

    my $cmd = "psql $psql_opts -c \"" . $sql . "\" $DB >> $REGRESS_LOG 2>&1";
    #print "CMD: " . $cmd . "\n";
    my $rv = system($cmd);
    if ( $rv ) {
      fail "Error encountered updating EXTENSION POSTGIS", $REGRESS_LOG;
      die;
    }

    # Handle raster split if coming from pre-split and going
    # to splitted raster
    if ( $OPT_UPGRADE_FROM && has_split_raster_ext($OPT_UPGRADE_TO) &&
         ! has_split_raster_ext($OPT_UPGRADE_FROM) )
    {
      if ( $OPT_WITH_RASTER )
      {
        # upgrade of postgis must have unpackaged raster, so
        # we create it again here
				my $sql = package_extension_sql('postgis_raster', ${nextver});

        print "Upgrading PostGIS Raster in '${DB}' using: ${sql}\n" ;

        my $cmd = "psql $psql_opts -c \"" . $sql . "\" $DB >> $REGRESS_LOG 2>&1";
        my $rv = system($cmd);
        if ( $rv ) {
          fail "Error encountered creating EXTENSION POSTGIS_RASTER from unpackaged on upgrade", $REGRESS_LOG;
          die;
        }
      }
      else
      {
        # Raster support was not requested, so drop it if
        # left unpackaged
        print "Packaging PostGIS Raster in '${DB}' for later drop using: ${sql}\n" ;

				$sql = package_extension_sql('postgis_raster', ${nextver});

        $cmd = "psql $psql_opts -c \"" . $sql . "\" $DB >> $REGRESS_LOG 2>&1";
        $rv = system($cmd);
        if ( $rv ) {
          fail "Error encountered creating EXTENSION POSTGIS_RASTER from unpackaged on upgrade", $REGRESS_LOG;
          die;
        }

        print "Dropping PostGIS Raster in '${DB}' using: ${sql}\n" ;

        $sql = "DROP EXTENSION postgis_raster";
        $cmd = "psql $psql_opts -c \"" . $sql . "\" $DB >> $REGRESS_LOG 2>&1";
        $rv = system($cmd);
        if ( $rv ) {
          fail "Error encountered dropping EXTENSION POSTGIS_RASTER on upgrade", $REGRESS_LOG;
          die;
        }
      }
    }

    if ( $upgrade_via_function )
    {
      # The function does everything
      return 1;
    }

    if ( $OPT_WITH_RASTER )
    {
        my $sql = "ALTER EXTENSION postgis_raster UPDATE TO '${nextver}'";

        if ( $OPT_UPGRADE_FROM =~ /^unpackaged/ ) {
					$sql = package_extension_sql('postgis_raster', ${nextver});
        }

        print "Upgrading PostGIS Raster in '${DB}' using: ${sql}\n" ;

        my $cmd = "psql $psql_opts -c \"" . $sql . "\" $DB >> $REGRESS_LOG 2>&1";
        my $rv = system($cmd);
        if ( $rv ) {
          fail "Error encountered updating EXTENSION POSTGIS_RASTER", $REGRESS_LOG;
          die;
        }
    }

    if ( $OPT_WITH_TOPO )
    {
      my $sql = "ALTER EXTENSION postgis_topology UPDATE TO '${nextver}'";

			if ( $OPT_UPGRADE_FROM =~ /^unpackaged/ ) {
				$sql = package_extension_sql('postgis_topology', ${nextver});
			}

      print "Upgrading PostGIS Topology in '${DB}' using: ${sql}\n";

      my $cmd = "psql $psql_opts -c \"" . $sql . "\" $DB >> $REGRESS_LOG 2>&1";
      my $rv = system($cmd);
      if ( $rv ) {
        fail "Error encountered updating EXTENSION POSTGIS_TOPOLOGY", $REGRESS_LOG;
        die;
      }
    }

    if ( $OPT_WITH_SFCGAL )
    {
			my $sql;

			if ( $OPT_UPGRADE_FROM =~ /^unpackaged/ ) {
				$sql = package_extension_sql('postgis_sfcgal', ${nextver});
			}
			elsif ( $OPT_UPGRADE_FROM && semver_lessthan($OPT_UPGRADE_FROM, "2.2.0") )
			{
				print "NOTICE: installing SFCGAL extension on upgrade "
						. "as it was not available in version '$OPT_UPGRADE_FROM'\n";
				$sql = "CREATE EXTENSION postgis_sfcgal VERSION '${nextver}'";
			}
			else
			{
				$sql = "ALTER EXTENSION postgis_sfcgal UPDATE TO '${nextver}'";
			}
      $cmd = "psql $psql_opts -c \"" . $sql . "\" $DB >> $REGRESS_LOG 2>&1";

			print "Upgrading PostGIS SFCGAL in '${DB}' using: ${sql}\n" ;

      $rv = system($cmd);
      if ( $rv ) {
        fail "Error encountered creating EXTENSION POSTGIS_SFCGAL", $REGRESS_LOG;
        die;
      }
    }

    return 1;
}

sub drop_spatial
{
	my $ok = 1;

  	if ( $OPT_WITH_TOPO )
	{
		load_sql_file("${STAGED_SCRIPTS_DIR}/uninstall_topology.sql");
	}
	if ( $OPT_WITH_RASTER )
	{
		load_sql_file("${STAGED_SCRIPTS_DIR}/uninstall_rtpostgis.sql");
	}
	if ( $OPT_WITH_SFCGAL )
	{
		load_sql_file("${STAGED_SCRIPTS_DIR}/uninstall_sfcgal.sql");
	}
	load_sql_file("${STAGED_SCRIPTS_DIR}/uninstall_postgis.sql");

  	return 1;
}

sub drop_spatial_extensions
{
    # ON_ERROR_STOP is used by psql to return non-0 on an error
    my $psql_opts="--no-psqlrc --variable ON_ERROR_STOP=true";
    my $ok = 1;
    my ($cmd, $rv);

    if ( $OPT_WITH_TOPO )
    {
        # NOTE: "manually" dropping topology schema as EXTENSION does not
        #       take care of that itself, see
        #       http://trac.osgeo.org/postgis/ticket/2138
        $cmd = "psql $psql_opts -c \"DROP EXTENSION postgis_topology; DROP SCHEMA topology;\" $DB >> $REGRESS_LOG 2>&1";
        $rv = system($cmd);
      	$ok = 0 if $rv;
    }

    if ( $OPT_WITH_SFCGAL )
    {
        $cmd = "psql $psql_opts -c \"DROP EXTENSION postgis_sfcgal;\" $DB >> $REGRESS_LOG 2>&1";
        $rv = system($cmd);
        $ok = 0 if $rv;
    }

    if ( $OPT_WITH_RASTER )
    {
        $cmd = "psql $psql_opts -c \"DROP EXTENSION IF EXISTS postgis_raster;\" $DB >> $REGRESS_LOG 2>&1";
        $rv = system($cmd);
      	$ok = 0 if $rv;
    }

    $cmd = "psql $psql_opts -c \"DROP EXTENSION postgis\" $DB >> $REGRESS_LOG 2>&1";
    $rv = system($cmd);
    if ( $rv ) {
        fail "Error encountered dropping EXTENSION POSTGIS", $REGRESS_LOG;
        die;
    }

    return $ok;
}

# Drop spatial from an existing database
sub uninstall_spatial
{
	my $ok;
	start_test("uninstall");

	if ( $OPT_EXTENSIONS )
	{
		$ok = drop_spatial_extensions();
	}
	else
	{
		$ok = drop_spatial();
	}

	if ( $ok )
	{
		show_progress(); # on to objects count
		$OBJ_COUNT_POST = count_db_objects();

		if ( $OBJ_COUNT_POST != $OBJ_COUNT_PRE )
		{
			fail("Object count pre-install ($OBJ_COUNT_PRE) != post-uninstall ($OBJ_COUNT_POST)");
			return 0;
		}
		else
		{
			pass("($OBJ_COUNT_PRE)");
			return 1;
		}
	}

	return 0;
}

# Dump and restore the database
sub dump_restore
{
  my $DBDUMP = $TMPDIR . '/' . $DB . ".dump";
  my $rv;

	print "Dumping and restoring database '${DB}'\n";

  $rv = system("pg_dump -Fc -f${DBDUMP} ${DB} >> $REGRESS_LOG 2>&1");
  if ( $rv ) {
    fail("Could not dump ${DB}", $REGRESS_LOG);
		die;
  }

  $rv = system("dropdb ${DB} >> $REGRESS_LOG 2>&1");
  if ( $rv ) {
    fail("Could not drop ${DB}", $REGRESS_LOG);
		die;
  }

  $rv = create_db();
  if ( $rv ) {
    fail("Could not create ${DB}", $REGRESS_LOG);
		die;
  }

  $rv = system("pg_restore -d ${DB} ${DBDUMP} >> $REGRESS_LOG 2>&1");
  if ( $rv ) {
    fail("Could not restore ${DB}", $REGRESS_LOG);
    die;
  }

  if ( $OPT_WITH_TOPO )
  {
    # We need to re-add "topology" to the search_path as it is lost
    # on dump/reload, see https://trac.osgeo.org/postgis/ticket/3454
    my $psql_opts = "--no-psqlrc --variable ON_ERROR_STOP=true";
    my $cmd = "psql $psql_opts -c \"SELECT topology.AddToSearchPath('topology')\" $DB >> $REGRESS_LOG 2>&1";
    $rv = system($cmd);
    if ( $rv ) {
      fail("Error encountered adding topology to search path after restore", $REGRESS_LOG);
      die;
    }
  }

  unlink($DBDUMP);

  return 1;
}

sub diff
{
	my ($expected_file, $obtained_file) = @_;
	my $diffstr = '';

	if ( $sysdiff ) {
		$diffstr = `diff --strip-trailing-cr -u $expected_file $obtained_file 2>&1`;
		return $diffstr;
	}

	open(OBT, $obtained_file) || return "Cannot open $obtained_file\n";
	open(EXP, $expected_file) || return "Cannot open $expected_file\n";
	my $lineno = 0;
	while (!eof(OBT) or !eof(EXP)) {
		# TODO: check for premature end of one or the other ?
		my $obtline=<OBT>;
		my $expline=<EXP>;
		$obtline =~ s/\r?\n$//; # Strip line endings
		$expline =~ s/\r?\n$//; # Strip line endings
		$lineno++;
		if ( $obtline ne $expline ) {
			my $diffln .= "$lineno.OBT: $obtline\n";
			$diffln .= "$lineno.EXP: $expline\n";
			$diffstr .= $diffln;
		}
	}
	close(OBT);
	close(EXP);
	return $diffstr;
}
