#!/usr/bin/env perl

#
# PostGIS - Spatial Types for PostgreSQL
# http://postgis.net
#
# Copyright (C) 2022 Regina Obe <lr@pcorp.us>
#
# This is free software; you can redistribute and/or modify it under
# the terms of the GNU General Public Licence. See the COPYING file.
#

#
# This script produces an .sql file containing
# CREATE OR REPLACE calls for each function
# in postgis.sql
#
# In addition, the transaction contains
# a check for Major postgis_lib_version()
# to match the one contained in lwpostgis.sql
#
# This never happens by just running make install
# as MODULE_FILENAME contains SO_MAJOR under
# all architectures.
#
#

eval "exec perl -w $0 $@"
  if (0);

use strict;
use warnings;

#
# Conditionally upgraded types and operators based
# on their last updated version and the version of
# the target database
#

sub parse_last_updated
{
    my $comment = shift;
    if ( $comment =~ m/.*(?:Availability|Changed|Updated):\s([^\.])\.([^.]*)/s )
    {
        return $1*100 + $2;
    }
    return 0;
}

sub parse_replaces
{
    my @replaces = ();
    my $comment = shift;
    my ($name, $args, $ver);
    foreach my $line ( split /\n/, $comment )
    {
        if ( $line =~ m/.*Replaces\s\s*([^\(]*)\(([^\)]*)\)\s\s*deprecated in\s\s*([^\.]*)\.([^.]*)/ )
        {
            $name = $1;
            $args = $2;
            $ver = $3*100 + $4;
            my @r = ($name, $args, $ver);
            push @replaces, \@r;
        }
    }
    return @replaces;
}

sub parse_missing
{
    my $comment = shift;
    my @missing = ();
    if ( $comment =~ m/.*(?:Missing in):\s([^\.])\.([^.]*)/s )
    {
        push(@missing, $1*100 + $2);
    }
    return join(',',@missing);
}

#
# Commandline argument handling
#
($#ARGV == 0)
  ||die
"Usage: perl create_upgrade.pl <create_script.sql> [<schema>]\nCreates a new SQL script that has all CREATE OR REPLACE converted to CREATE so it is suitable for CREATE EXTENSION only.\n"
  if ( @ARGV < 1 || @ARGV > 3 );

my $sql_file = $ARGV[0];

die "Unable to open input SQL file $sql_file\n"
  if ( !-f $sql_file );

#
# Go through the SQL file and strip out objects that cannot be
# applied to an existing, loaded database: types and operators
# and operator classes that have already been defined.
#
my $comment = '';
open( INPUT, $sql_file ) || die "Couldn't open file: $sql_file\n";
while(<INPUT>)
{

    if (/^create or replace function/i)
    {
        my $def .= $_;
        my $endfunc = 0;
		$def =~ s/CREATE OR REPLACE/CREATE/;
		#print "-- entering create or replace loop\n";
        while(<INPUT>)
        {
            $def .= $_;
            $endfunc = 1 if /^\s*(\$\$\s*)?LANGUAGE /i;

            last if ( $endfunc && /\;/ );
        }

        print $def;
		#print "-- exiting create or replace loop\n";
    }
	elsif (/^do *language .*\$\$/i)
    {
        print;
        while(<INPUT>)
        {
            print;
            last if /\$\$/;
        }
    }

    # Always output create ore replace view (see ticket #1097)
    elsif (/^create or replace view\s+(\S+)\s*/i)
    {
		my $def = $_;
        $def = $_;
		$def =~ s/CREATE OR REPLACE/CREATE/;
        while(<INPUT>)
        {
            $def .= $_;
            last if /\;\s*$/;
        }
		print $def;
    }

    # Always output create ore replace rule
    elsif (/^create or replace rule\s+(\S+)\s*/i)
    {
		my $def = $_;
		$def = $_;
		$def =~ s/CREATE OR REPLACE/CREATE/;
        while(<INPUT>)
        {
            $def .= $_;
            last if /\;\s*$/;
        }
		print $def;
    }
	else {
		print;
	}
}

close(INPUT);

#print "COMMIT;\n";

1;

__END__

