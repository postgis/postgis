#!/usr/bin/env perl

#
# PostGIS - Spatial Types for PostgreSQL
# http://postgis.net
#
# Copyright (C) 2016 Sandro Santilli <strk@kbt.io>
#
# This is free software; you can redistribute and/or modify it under
# the terms of the GNU General Public Licence. See the COPYING file.
#

use warnings;
use strict;

(@ARGV) ||
  die "Usage: perl $0 <spatial_ref_sys>\n" .
      "Creates an SQL script to mark system records of spatial_ref_sys.\n";

my $SRSFILE=$ARGV[0];

open SRSFILE, "<$SRSFILE" || die "Can't open $SRSFILE for reading\n";

my @SRIDS = ();
while (<SRSFILE>) {
  /^INSERT/ || next;
  /\(([0-9]*),/ || next;
  push @SRIDS, $1;
}
@SRIDS = sort {$b <=> $a} @SRIDS;

print "SELECT pg_catalog.pg_extension_config_dump('spatial_ref_sys',";
print " 'WHERE NOT (\n";
my $OR="";
while (@SRIDS)
{
  my $min = pop @SRIDS;
  #print "min:$min\n";
  # Find upper bound
  my $max = $min;
  while (@SRIDS)
  {
    #print "next:$SRIDS[@SRIDS-1]\n";
    last if ( $SRIDS[@SRIDS-1] != $max+1 );
    $max = pop @SRIDS
  };
  if ( $min != $max ) {
    print $OR . "srid BETWEEN $min AND $max\n";
  } else {
    print $OR . "srid = $min\n";
  }
  $OR = "OR ";
}
print join ",\n", @SRIDS;
print ")');"
#mark_editable_objects.sql.in
