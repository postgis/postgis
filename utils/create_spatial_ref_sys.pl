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
	die "Usage: perl $0 -u <spatial_ref_sys>\n" .
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

while(<SRSFILE>) {
	chop;
	my ($srid, $auth_name, $auth_srid, $srtext, $proj4text) = split (/\t/);
	$auth_name =~ s/\'/\'\'/g;
	$srtext =~ s/\'/\'\'/g;
	$proj4text =~ s/\'/\'\'/g;
	print "INSERT INTO \"$SRSTABLE\" (\"srid\", \"auth_name\", \"auth_srid\", \"srtext\", \"proj4text\") VALUES ($srid, '$auth_name', $auth_srid, '$srtext', '$proj4text')";
	if ($UPDATE) {
		print "\nON CONFLICT (srid) DO NOTHING";
	}
	print ";\n\n";
}

print "COMMIT;\n";
print "ANALYZE \"$SRSTABLE\";\n";
