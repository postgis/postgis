#!/usr/bin/env perl

#
# PostGIS - Spatial Types for PostgreSQL
# http://postgis.net
#
# Copyright (C) 2023 Sandro Santilli <strk@kbt.io>
#
# This is free software; you can redistribute and/or modify it under
# the terms of the GNU General Public Licence. See the COPYING file.
#
#---------------------------------------------------------------------
#
# This script is aimed at generating a list of objects
# signatures used by postgis_restore.pl to decide
# which objects belong to PostGIS
#
#---------------------------------------------------------------------

use warnings;
use strict;

my $me = $0;

my $usage = qq{
Usage:	$me [<sqlfile> ...]
        Reads sql statements from given files or standard input
				and generates a list of signatures from DROP lines.

};

my %reserved_sql_word = (
	'double' => 1,
	'character' => 1
);

# Example:
#  INPUT: inout first double precision, second integer, OUT third text, fourth bool
# OUTPUT: first double precision, second integer, fourth bool
sub clean_inout_arguments {
	my @args = @_;
	my @out;
	#print "XXX to strip: " . join(',', @args) . "\n";
	foreach ( @args )
	{
		my $a = $_;

		#print "  XXX arg: [$a]\n";
		# If the arg is composed by multiple words
		# check for out and inout indicators
		if ( $a =~ m/([^ ]*) (.*)/ )
		{
			# Skip this arg if out only
			next if $1 eq 'out';

			# Hide the inout indicator
			$a = $2 if $1 eq 'inout';
		}
		#print "  XXX arg became: $a\n";
		push @out, $a;
	}
	#print "XXX striped: " . join(',', @out) . "\n";
	return @out;
}

# Example:
#  INPUT: int,named double precision,named text
# OUTPUT: int,double precision,text
sub strip_argument_names {
	my @args = @_;
	my @out;
	#print "XXX to strip: " . join(',', @args) . "\n";
	foreach ( @args )
	{
		my $a = $_;

		#print "  XXX arg: $a\n";
		# If the arg is composed by multiple words
		# drop the first, unless it's a reserved word
		if ( $a =~ m/([^ ]*) (.*)/ )
		{
			unless ( $reserved_sql_word{$1} )
			{
				$a = $2;
				#print "  XXX arg became: $a\n";
			}
		}
		push @out, $a;
	}
	#print "XXX striped: " . join(',', @out) . "\n";
	return @out;
}

sub canonicalize_args {
	my @args = @_;
	my @out;
	foreach ( @args )
	{
		my $a = $_;
		$a =~ s/varchar/character varying/g;
		$a =~ s/float8/double precision/g;
		$a =~ s/\bint\b/integer/g;
		$a =~ s/\bint4\b/integer/g;
		$a =~ s/\bint8\b/bigint/g;
		$a =~ s/\bchar\b/character/g;
		$a =~ s/\bbool\b/boolean/g;
		push @out, $a;
	}
	return @out;
}

sub handle_function_signature {
	my $line = shift;

	$line = lc($line);
	$line =~ s/topology\.//g;

	$line =~ m/ *([^\( ]*) *\((.*)\)/ or die "Unexpected FUNCTION signature: $line";

	my $name = $1;
	my $args = $2;
	$args =~ s/\s*$//; # trim trailing blanks
	$args =~ s/^\s*//; # trim leading blanks

	my @args = split('\s*,\s*', $args);
	@args = canonicalize_args(@args);

	print "COMMENT FUNCTION $name(" . join(', ', @args) .")\n";

	# Example manifest line for comments on function with inout params:
	# 4247; 0 0 COMMENT public FUNCTION testinoutmix(INOUT "inout" double precision, second integer, OUT thirdout integer, fourth integer) strk

	# Example manifest line for function with inout params:
	# 955; 1255 27730785 FUNCTION public testinoutmix(double precision, integer, integer) strk

	# No inout indicator or out parameters for function signatures
	my @inonly_args = clean_inout_arguments(@args);

	# For *function* signature we are supposed to strip argument names
	my @unnamed_args = strip_argument_names(@inonly_args);

	print "FUNCTION $name(" . join(', ', @unnamed_args) . ")\n";
}

while (<>)
{
	#print "XXX 0 $_";

	# type signature
	if ( /^DROP TYPE IF EXISTS\s\s*([^\s]*)/ )
	{
		my $t = lc($1);
		$t =~ s/topology\.//g;
		print "COMMENT TYPE $t\n";
		print "TYPE $t\n";
	}

	# aggregate signature
	if ( /^DROP AGGREGATE IF EXISTS\s+(.*)\((.*)\)/ )
	{
		my $name = lc($1);
		my $args = lc($2);

		s/^\s+//, s/\s+$// for $name;
		$name =~ s/topology\.//g;

		$args =~ s/topology\.//g;
		my @args = split('\s*,\s*', $args);
		@args = canonicalize_args(@args);

		print "COMMENT AGGREGATE $name(" . join(', ', @args) . ")\n";

		# For *aggregate* signature we are supposed to strip
		# also argument names, which aint easy
		my @unnamed_args = strip_argument_names(@args);

		print "AGGREGATE $name(" . join(', ', @unnamed_args) . ")\n";
	}

	# Function signature
	elsif ( /^DROP FUNCTION IF EXISTS/ )
	{
		my $origline = $_;
		my $line = $origline;

		$line =~ s/DROP FUNCTION IF EXISTS //;

		# We don't want to ALWAYS strip comments
		# because we might handle 'Replaces' comment
		# at some point
		$line =~ s/ *--.*//;

		handle_function_signature($line);
	}

	# Deprecated function signature
	# EXAMPLE: ALTER FUNCTION _st_concavehull( geometry ) RENAME TO _st_concavehull_deprecated_by_postgis_303;
	elsif ( /ALTER FUNCTION .* RENAME TO .*_deprecated_by_postgis_/ )
	{
		my $origline = $_;
		my $line = $origline;

		$line =~ s/ *ALTER FUNCTION (.*) RENAME TO .*_deprecated_by_postgis_.*/$1/;

		handle_function_signature($line);
	}

	# Deprecated function signature using
	# _postgis_drop_function_by_signature
	# EXAMPLE: SELECT _postgis_drop_function_by_signature('pgis_geometry_union_finalfn(internal)');
	elsif ( /SELECT _postgis_drop_function_by_signature\('[^']*'\);/ )
	{
		my $origline = $_;
		my $line = $origline;

		$line =~ s/SELECT _postgis_drop_function_by_signature\('([^']*)'\);/$1/;

		handle_function_signature($line);
	}
}
