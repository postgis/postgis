#!/usr/bin/perl

#
# PostGIS - Spatial Types for PostgreSQL
# http://postgis.net
#
# Copyright (C) 2020 Sandro Santilli <strk@kbt.io>
#
# This is free software; you can redistribute and/or modify it under
# the terms of the GNU General Public Licence. See the COPYING file.
#


sub usage
{
	print qq{Usage: $0 <command> [<args>]
Commands:
  help               print this message and exits
  enable <database>  enable postgis in given database
  upgrade <database>  enable postgis in given database
  status <database>  prints postgis status in given database
};

}


sub enable
{
	print "Enable called with args: @_\n";
  my $db = shift;
  die "Please specify a database name" unless "$db";
  die "Enable is not implemented yet";
}

sub upgrade
{
  die "Please specify at least a database name" unless @_;

  foreach my $db (@_)
  {
    print "upgrading db $db\n";
    my $LOG=`cat <<EOF | psql -qXtA ${db}
BEGIN;
UPDATE pg_catalog.pg_extension SET extversion = 'ANY'
 WHERE extname IN (
			'postgis',
			'postgis_raster',
			'postgis_sfcgal',
			'postgis_topology',
			'postgis_tiger_geocoder'
  );
SELECT postgis_extensions_upgrade();
COMMIT;
EOF`;
  }
}

sub status
{
  die "Please specify at least a database name" unless @_;

  foreach my $db (@_)
  {
    my $sql = "
SELECT n.nspname
  FROM pg_namespace n, pg_proc p
  WHERE n.oid = p.pronamespace
    AND p.proname = 'postgis_full_version'
";

		my $SCHEMA=`psql -qXtA ${db} -c "${sql}" 2>&1`;
		chop($SCHEMA);
		if ( $? ne 0 )
		{
			$SCHEMA =~ s/^.*FATAL: *//;
			print "db $db cannot be inspected: $SCHEMA\n";
			next;
		}

    if ( ! "$SCHEMA" )
		{
      print "db $db does not have postgis enabled\n";
      next;
    }

    $sql="SELECT ${SCHEMA}.postgis_full_version()";
    my $FULL_VERSION=`psql -qXtA ${db} -c "${sql}"`;
		#print "FULL_VERSION: ${FULL_VERSION}\n";

    #POSTGIS="3.1.0dev r3.1.0alpha1-3-gfc5392de7"
    my $VERSION="unknown";
    if ( $FULL_VERSION =~ /POSTGIS="([^ ]*).*/ )
		{
			$VERSION=$1;
		}

    my $EXTENSION='';
    if ( $FULL_VERSION =~ /\[EXTENSION\]/ )
		{
      $EXTENSION=" as extension";
    }
    my $NEED_UPGRADE='';
    if ( $FULL_VERSION =~ /need upgrade/ )
		{
      $NEED_UPGRADE=" - NEEDS UPGRADE";
    }

    print "db $db has postgis ${VERSION}${EXTENSION} in schema ${SCHEMA}${NEED_UPGRADE}\n";
  }
}

# Avoid NOTICE messages
$ENV{'PGOPTIONS'} = $ENV{'PGOPTIONS'} . ' --client-min-messages=warning';

my $cmd = shift;
if ( $cmd eq "help" ) {
	usage(STDOUT);
	exit 0;
}
elsif ( $cmd eq "enable" ) {
	exit enable(@ARGV);
}
elsif ( $cmd eq "upgrade" ) {
	exit upgrade(@ARGV);
}
elsif ( $cmd eq "status" ) {
	exit status(@ARGV);
}
else {
	print STDERR "Unrecognized command: $cmd\n";
  usage(STDERR);
  exit 1
}
