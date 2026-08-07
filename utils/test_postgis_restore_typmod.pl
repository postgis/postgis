#!/usr/bin/env perl

#
# PostGIS - Spatial Types for PostgreSQL
# http://postgis.net
#
# Copyright (C) 2026 Darafei Praliaskouski <me@komzpa.net>
#
# This is free software; you can redistribute and/or modify it under
# the terms of the GNU General Public Licence. See the COPYING file.
#

use warnings;
use strict;

use File::Temp qw(tempdir);

my $restore_script = shift @ARGV
  or die "Usage: $0 <postgis_restore.pl>\n";

sub write_file
{
	my ($path, $contents) = @_;
	open(my $fh, '>', $path) || die "Could not write $path: $!\n";
	print {$fh} $contents;
	close($fh) || die "Could not close $path: $!\n";
}

sub install_fake_programs
{
	my ($bindir) = @_;

	write_file("$bindir/pg_dump", <<'SH');
#!/bin/sh
echo "pg_dump (fake)"
SH
	chmod 0755, "$bindir/pg_dump";

	write_file("$bindir/pg_restore", <<'SH');
#!/bin/sh
for arg in "$@"; do
	if test "$arg" = "--version"; then
		echo "pg_restore (fake)"
		exit 0
	fi
	if test "$arg" = "-l"; then
		cat "$POSTGIS_RESTORE_TEST_TOC"
		exit 0
	fi
done
cat "$POSTGIS_RESTORE_TEST_SQL"
SH
	chmod 0755, "$bindir/pg_restore";
}

sub run_restore
{
	my ($extra_arg, $sql) = @_;
	my $dir = tempdir(CLEANUP => 1);
	my $bindir = "$dir/bin";
	mkdir $bindir || die "Could not create $bindir: $!\n";
	install_fake_programs($bindir);

	my $dump = "$dir/test.dump";
	my $toc_file = "$dir/toc.txt";
	my $sql_file = "$dir/restore.sql";
	write_file($dump, "fake dump\n");
	write_file($toc_file, "1; 0 0 TABLE DATA public spatial_ref_sys postgres\n");
	write_file($sql_file, $sql);

	local $ENV{PATH} = "$bindir:$ENV{PATH}";
	local $ENV{POSTGIS_RESTORE_TEST_TOC} = $toc_file;
	local $ENV{POSTGIS_RESTORE_TEST_SQL} = $sql_file;

	my @args = ($^X, $restore_script);
	push @args, $extra_arg if defined $extra_arg;
	push @args, $dump;

	open(my $out, '-|', @args)
	  || die "Could not run $restore_script: $!\n";
	my $stdout = do { local $/; <$out> };
	close($out) || die "$restore_script failed\n";

	return $stdout;
}

sub assert_contains
{
	my ($text, $needle, $label) = @_;
	die "not ok - $label\nMissing: $needle\nIn:\n$text\n"
	  if index($text, $needle) < 0;
}

sub assert_not_contains
{
	my ($text, $needle, $label) = @_;
	die "not ok - $label\nUnexpected: $needle\nIn:\n$text\n"
	  if index($text, $needle) >= 0;
}

my $restore_sql = <<'SQL';
CREATE TABLE public.t (
    id integer,
    geom public.geometry,
    CONSTRAINT enforce_dims_geom CHECK ((public.st_ndims(geom) = 2)),
    CONSTRAINT enforce_geotype_geom CHECK (((public.geometrytype(geom) = 'POINT'::text) OR (geom IS NULL))),
    CONSTRAINT enforce_srid_geom CHECK ((public.st_srid(geom) = 4326))
);

CREATE TABLE public.quoted (
    "ID" integer,
    "G" public.geometry,
    CONSTRAINT "enforce_dims_G" CHECK ((public.st_ndims("G") = 2)),
    CONSTRAINT "enforce_geotype_G" CHECK (((public.geometrytype("G") = 'MULTIPOLYGON'::text) OR ("G" IS NULL))),
    CONSTRAINT "enforce_srid_G" CHECK ((public.st_srid("G") = (-1)))
);

CREATE TABLE public.xyz (
    id integer,
    geom3 public.geometry,
    CONSTRAINT enforce_dims_geom3 CHECK ((public.st_ndims(geom3) = 3)),
    CONSTRAINT enforce_geotype_geom3 CHECK (((public.geometrytype(geom3) = 'LINESTRING'::text) OR (geom3 IS NULL))),
    CONSTRAINT enforce_srid_geom3 CHECK ((public.st_srid(geom3) = 4326))
);

CREATE TABLE public.with_opts (
    id integer,
    geom public.geometry,
    CONSTRAINT enforce_dims_geom CHECK ((public.st_ndims(geom) = 2)),
    CONSTRAINT enforce_geotype_geom CHECK (((public.geometrytype(geom) = 'POINT'::text) OR (geom IS NULL))),
    CONSTRAINT enforce_srid_geom CHECK ((public.st_srid(geom) = 4326))
)
WITH (fillfactor='70');
SQL

my $stdout = run_restore(undef, $restore_sql);
assert_contains($stdout, 'geom public.geometry,', 'leave unrequested geometry column unconstrained');
assert_contains($stdout, 'CONSTRAINT enforce_dims_geom CHECK ((public.st_ndims(geom) = 2))', 'leave constraints without opt-in');

$stdout = run_restore('--convert-to-typmod', $restore_sql);
assert_contains($stdout, 'geom public.geometry(POINT,4326)', 'convert simple 2D geometry column to typmod');
assert_not_contains($stdout, 'CONSTRAINT enforce_dims_geom CHECK ((public.st_ndims(geom) = 2))', 'drop converted dims constraint');
assert_not_contains($stdout, 'CONSTRAINT enforce_geotype_geom CHECK (((public.geometrytype(geom) = \'POINT\'::text) OR (geom IS NULL)))', 'drop converted geotype constraint');
assert_not_contains($stdout, 'CONSTRAINT enforce_srid_geom CHECK ((public.st_srid(geom) = 4326))', 'drop converted srid constraint');
assert_contains($stdout, '"G" public.geometry(MULTIPOLYGON,0)', 'convert quoted geometry column and clamp unknown SRID');
assert_not_contains($stdout, 'CONSTRAINT "enforce_srid_G"', 'drop converted quoted SRID constraint');
assert_contains($stdout, 'geom3 public.geometry,', 'leave non-2D geometry column constraint-based');
assert_contains($stdout, 'CONSTRAINT enforce_dims_geom3 CHECK ((public.st_ndims(geom3) = 3))', 'keep non-2D dimensions constraint');
assert_contains($stdout, "geom public.geometry(POINT,4326)\n)\nWITH (fillfactor='70');", 'preserve table trailer after typmod conversion');
assert_not_contains($stdout, "geom public.geometry(POINT,4326),\n)\nWITH", 'drop trailing comma before table trailer');

print "ok - postgis_restore typmod conversion tests\n";
