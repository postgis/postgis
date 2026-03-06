#!/bin/sh

set -eu

CODESPELL=${1:-codespell}

ROOT=$(git rev-parse --show-toplevel 2>/dev/null || pwd)
cd "$ROOT"

tmpfiles=$(mktemp)
trap 'rm -f "$tmpfiles"' EXIT HUP INT TERM

# Keep the check deterministic by starting from tracked files and excluding
# vendor data, generated trees, and authoritative datasets that are not prose.
git ls-files -z -- . \
	':(exclude)deps' \
	':(exclude)extensions/address_standardizer' \
	':(exclude)doc/html' \
	':(exclude)doc/po' \
	':(exclude)doc/postgis-out.xml' \
	':(exclude)doc/postgis-nospecial.xml' \
	':(exclude)aclocal.m4' \
	':(exclude)configure' \
	':(exclude)macros/libtool.m4' \
	':(exclude)spatial_ref_sys.sql' \
	':(exclude)extensions/postgis/sql/spatial_ref_sys.sql' \
	':(glob,exclude)extensions/postgis/sql/postgis--*.sql' \
	':(glob,exclude)**/*.po' \
	> "$tmpfiles"

# The comment SQL files are generated from the docs and shipped in release
# artifacts, so include them when they exist.
for generated in doc/postgis_comments.sql \
	doc/raster_comments.sql \
	doc/sfcgal_comments.sql \
	doc/tiger_geocoder_comments.sql \
	doc/topology_comments.sql
do
	if [ -f "$generated" ]; then
		printf '%s\0' "$generated" >> "$tmpfiles"
	fi
done

xargs -0 "$CODESPELL" --config .codespellrc < "$tmpfiles"
