#!/usr/bin/perl

use File::Basename;
use File::Temp 'tempdir';
#use File::Which;
use File::Copy;
use File::Path;
use Cwd 'abs_path';
use Getopt::Long;
use strict;


##################################################################
# Usage ./run_test.pl <testname> [<testname>]
#
#  Create the spatial database 'postgis_reg' (or whatever $DB 
#  is set to) if it doesn't already exist.
#
#  Run the <testname>.sql script
#  Check output against <testname>_expected
##################################################################

if ( @ARGV < 1 )
{
	usage();
}


##################################################################
# Global configuration items
##################################################################

my $DB = "postgis_reg";
my $REGDIR = abs_path(dirname($0));
my $SHP2PGSQL = $REGDIR . "/../loader/shp2pgsql";
my $PGSQL2SHP = $REGDIR . "/../loader/pgsql2shp";
my $RASTER2PGSQL = $REGDIR . "/../raster/loader/raster2pgsql";


##################################################################
# Parse command line opts
##################################################################

my $OPT_CLEAN = 0;
my $OPT_NODROP = 0;
my $OPT_NOCREATE = 0;
my $OPT_UPGRADE = 0;
my $OPT_WITH_TOPO = 0;
my $OPT_WITH_RASTER = 0;
my $OPT_EXPECT = 0;
my $OPT_EXTENSIONS = 0;
my $VERBOSE = 0;

GetOptions (
	'verbose' => \$VERBOSE,
	'clean' => \$OPT_CLEAN,
	'nodrop' => \$OPT_NODROP, 
	'upgrade' => \$OPT_UPGRADE,
	'nocreate' => \$OPT_NOCREATE,
	'topology' => \$OPT_WITH_TOPO,
	'raster' => \$OPT_WITH_RASTER,
	'expect' => \$OPT_EXPECT,
	'extensions' => \$OPT_EXTENSIONS
	);


##################################################################
# Set the locale to "C" so error messages match
# Save original locale to set back
##################################################################

my $ORIG_LC_ALL = $ENV{"LC_ALL"};
my $ORIG_LANG = $ENV{"LANG"};
$ENV{"LC_ALL"} = "C";
$ENV{"LANG"} = "C";

# Add locale info to the psql options
my $PGOPTIONS = $ENV{"PGOPTIONS"} . " -c lc_messages=C";
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

#foreach my $exec ( ("psql", "createdb", "createlang", "dropdb") )
#{
#	my $execdir = which( $exec );
#	print "Checking for $exec ... ";
#	if ( $execdir )
#	{
#		print "found $execdir\n";
#	}
#	else
#	{
#		print "failed\n";
#		die "Unable to find $exec executable. Please ensure it is on your PATH.\n";
#	}
#}

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
if ( -d "/tmp/" && -w "/tmp/" )
{
	$TMPDIR = "/tmp/pgis_reg";
}
else
{
	$TMPDIR = tempdir( CLEANUP => 0 );
}

mkdir $TMPDIR if ( ! -d $TMPDIR );

# Set log name
my $REGRESS_LOG = "${TMPDIR}/regress_log";

# Report
print "TMPDIR is $TMPDIR\n";


##################################################################
# Prepare the database
##################################################################

my @dblist = grep(/$DB/, split(/\n/, `psql -l`));
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
		print "Database $DB already exists.\n";
		print "Drop it, or run with the --nocreate flag to use it.\n";
		exit(1);
	}
}

if ( $OPT_UPGRADE )
{
	upgrade_spatial();
}


##################################################################
# Report PostGIS environment
##################################################################

my $libver = sql("select postgis_lib_version()");

if ( ! $libver )
{
	`dropdb $DB`;
	print "\nSomething went wrong (no PostGIS installed in $DB).\n";
	print "For details, check $REGRESS_LOG\n\n";
	exit(1);
}

my $geosver =  sql("select postgis_geos_version()");
my $projver = sql("select postgis_proj_version()");
my $svnrev = sql("select postgis_svn_version()");
my $libbuilddate = sql("select postgis_lib_build_date()");
my $pgsqlver = sql("select version()");
my $gdalver = sql("select postgis_gdal_version()") if $OPT_WITH_RASTER;

print "$pgsqlver\n";
print "  Postgis $libver - r${svnrev} - $libbuilddate\n";
print "  GEOS: $geosver\n" if $geosver;
print "  PROJ: $projver\n" if $projver;
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
	# catch a common mistake (strip trailing .sql)
	$TEST =~ s/.sql$//;

	start_test($TEST);

	# Check for a "-pre.pl" file in case there are setup commands 
    eval_file("${TEST}-pre.pl");

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
	if ( -r "${TEST}.dbf" )
	{
		pass() if ( run_loader_test() );
	}
	elsif ( -r "${TEST}.tif" )
	{
		my $rv = run_raster_loader_test();
		pass() if $rv;
	}
	elsif ( -r "${TEST}.sql" )
	{
		my $rv = run_simple_test("${TEST}.sql", "${TEST}_expected");
		pass() if $rv;
	}
	else
	{
		print " skipped (can't read ${TEST}.sql)\n";
		$SKIP++;
		next;
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
	sleep(1);
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
Usage: $0 <testname> [<testname>]
Options:
  --verbose    be verbose about failures
  --nocreate   do not create the regression database on start
  --upgrade    source the upgrade scripts on start
  --nodrop     do not drop the regression database on exit
  --raster     load also raster extension
  --topology   load also topology extension
  --clean      cleanup test logs on exit
  --expect     save obtained output as expected
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
	my $cmd = "psql -v \"VERBOSITY=terse\" -tXA $DB < $sql > $tmpfile 2>&1";
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
	my $cmd = "psql -tXA -d $DB -c \"DROP TABLE IF EXISTS $tblname\" >> $REGRESS_LOG 2>&1";
	my $rv = system($cmd);
	die "Could not run: $cmd\n" if $rv;	
}

sub sql
{
	my $sql = shift;
	my $result = `psql -tXA -d $DB -c "$sql"`;
	$result =~ s/\n$//;
	$result;
}

sub eval_file
{
    my $file = shift;
    my $pl;
    if ( -r $file )
    {
        open(PL, $file);
        $pl = <PL>;
        close(PL);
        eval($pl);
    }
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

	my $cmd = "psql -v \"VERBOSITY=terse\" -v \"tmpfile='$tmpfile'\" -tXA $DB < $sql > $outfile 2>&1";
	my $rv = system($cmd);

	# Check for ERROR lines
	open(FILE, "$outfile");
	my @lines = <FILE>;
	close(FILE);

	# Strip the lines we don't care about
	@lines = grep(!/^\$/, @lines);
	@lines = grep(!/^(INSERT|DELETE|UPDATE|SELECT)/, @lines);
	@lines = grep(!/^(CONTEXT|RESET|ANALYZE)/, @lines);
	@lines = grep(!/^(DROP|CREATE|VACUUM)/, @lines);
	@lines = grep(!/^(SET|TRUNCATE)/, @lines);
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
		print " expected\n";
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
			fail(" $description: running shp2pgsql", "$errfile");
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
# $3 - If you pass true, this will run the loader even if neither
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
		$cmd = "$RASTER2PGSQL $loader_options -C -f the_rast ${TEST}.tif $tblname > $outfile 2> $errfile";
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
		my @opts = <FILE>;
		close(FILE);
		@opts = grep(!/^\s*#/, @opts);
		map(s/\n//, @opts);
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
			where nspname NOT LIKE 'pg_%' union all
		select count(*) from pg_opfamily ) 
		select sum(count) from counts");

 	return $count;
}


##################################################################
# Create the spatial database
##################################################################
sub create_spatial 
{
	my ($cmd, $rv);
	print "Creating database '$DB' \n";

	$cmd = "createdb --encoding=UTF-8 --template=template0 --lc-collate=C $DB > $REGRESS_LOG";
	$rv = system($cmd);
	$cmd = "createlang plpgsql $DB >> $REGRESS_LOG 2>&1";
	$rv = system($cmd);

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
		my $cmd = "psql $psql_opts -Xf $file $DB >> $REGRESS_LOG 2>&1";
		print "  $file\n" if $VERBOSE;
		my $rv = system($cmd);
		die "\nError encountered loading $file, see $REGRESS_LOG for details\n\n"
		    if $rv;
	}
	return 1;
}

# Prepare the database for spatial operations (extension method)
sub prepare_spatial_extensions
{
	print "Preparing db '${DB}' using 'CREATE EXTENSION'\n"; 

	# ON_ERROR_STOP is used by psql to return non-0 on an error
	my $psql_opts = "--no-psqlrc --variable ON_ERROR_STOP=true";
    my $cmd = "psql $psql_opts -c \"CREATE EXTENSION postgis\" $DB >> $REGRESS_LOG 2>&1";
    my $rv = system($cmd);

	die "\nError encountered creating EXTENSION POSTGIS, see $REGRESS_LOG for details\n\n"
	    if $rv;

    if ( $OPT_WITH_TOPO )
    {
        $cmd = "psql $psql_opts -c \"CREATE EXTENSION postgis_topology\" $DB >> $REGRESS_LOG 2>&1";
        $rv = system($cmd);
    	die "\nError encountered creating EXTENSION POSTGIS_TOPOLOGY, see $REGRESS_LOG for details\n\n"
    	    if $rv;
    }
    return 1;
}

# Prepare the database for spatial operations (old method)
sub prepare_spatial
{
	print "Loading PostGIS into '${DB}' \n";

	# Load postgis.sql into the database
	load_sql_file("${STAGED_SCRIPTS_DIR}/postgis.sql", 1);
	load_sql_file("${STAGED_SCRIPTS_DIR}/postgis_comments.sql", 0);
	
	if ( $OPT_WITH_TOPO )
	{
		load_sql_file("${STAGED_SCRIPTS_DIR}/topology.sql", 1);
		load_sql_file("${STAGED_SCRIPTS_DIR}/topology_comments.sql", 0);
	}
	
	if ( $OPT_WITH_RASTER )
	{
		load_sql_file("${STAGED_SCRIPTS_DIR}/rtpostgis.sql", 1);
		load_sql_file("${STAGED_SCRIPTS_DIR}/raster_comments.sql", 0);
	}

	return 1;
}

# Upgrade an existing database (soft upgrade)
sub upgrade_spatial
{
    print "Upgrading PostGIS in '${DB}' \n" ;

    my $script = `ls ${STAGED_SCRIPTS_DIR}/postgis_upgrade_*_minor.sql`;
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
        my $script = `ls ${STAGED_SCRIPTS_DIR}/topology_upgrade_*_minor.sql`;
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
        my $script = `ls ${STAGED_SCRIPTS_DIR}/rtpostgis_upgrade_*_minor.sql`;
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
        $cmd = "psql $psql_opts -c \"DROP EXTENSION postgis_topology\" $DB >> $REGRESS_LOG 2>&1";
        $rv = system($cmd);
      	$ok = 0 if $rv;
    }
    
    $cmd = "psql $psql_opts -c \"DROP EXTENSION postgis\" $DB >> $REGRESS_LOG 2>&1";
    $rv = system($cmd);
  	die "\nError encountered dropping EXTENSION POSTGIS, see $REGRESS_LOG for details\n\n"
  	    if $rv;

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

sub diff
{
	my ($obtained_file, $expected_file) = @_;
	my $diffstr = '';

	open(OBT, $obtained_file) || die "Cannot open $obtained_file\n";
	open(EXP, $expected_file) || die "Cannot open $expected_file\n";
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
