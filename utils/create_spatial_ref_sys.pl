#!/usr/bin/env perl

#
# PostGIS - Spatial Types for PostgreSQL
# http://postgis.net
#
# Copyright (C) 2022 Paul Ramsey <pramsey@cleverelephant.ca>
#
# This is free software; you can redistribute and/or modify it under
# the terms of the GNU General Public Licence. See the COPYING file.
#

use warnings;
use strict;

(@ARGV) ||
	die "Usage: perl $0 <spatial_ref_sys>\n" .
		"Creates an SQL script load or update records of spatial_ref_sys.\n";

my $UPDATE = 0;
my $SRSFILE = 0;
my $SRSTABLE = "spatial_ref_sys";

for (my $i=0; $i<@ARGV; $i++)
{
	if ( $ARGV[$i] =~ m/^-/ )
	{
		if ( $ARGV[$i] eq '-u' )
		{
			$UPDATE = 1;
		}
	}
	elsif ( ! $SRSFILE )
	{
		$SRSFILE = $ARGV[$i];
	}
}

open SRSFILE, "<$SRSFILE" || die "Can't open $SRSFILE for reading\n";

print "BEGIN;\n";
print "INSERT INTO \"$SRSTABLE\" (\"srid\", \"auth_name\", \"auth_srid\", \"srtext\", \"proj4text\") VALUES\n";
my $count = 0;
while(<SRSFILE>) {
	chop;
	my ($srid, $auth_name, $auth_srid, $srtext, $proj4text) = split (/\t/);
	$auth_name =~ s/\'/\'\'/g;
	$srtext =~ s/\'/\'\'/g;
	$proj4text =~ s/\'/\'\'/g;
	print ",\n" if $count++;
	print "($srid, '$auth_name', $auth_srid, '$srtext', '$proj4text')";
}

print "\nON CONFLICT (srid) DO NOTHING;\n";


# INSERT INTO spatial_ref_sys (srid, auth_name, auth_srid, srtext, proj4text)
# VALUES
# (200003, 'test3', 3, 'test3', 'test3'),
# (200002, 'test2', 2, 'test2', 'test2'),
# (200001, 'test1', 1, 'test1', 'test1')
# ON CONFLICT DO NOTHING;

print "COMMIT;\n";
print "ANALYZE \"$SRSTABLE\";\n";
