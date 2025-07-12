#!/usr/bin/env perl

#
# PostGIS - Spatial Types for PostgreSQL
# http://postgis.net
#
# Copyright (C) 2014-2023 Sandro Santilli <strk@kbt.io>
# Copyright (C) 2009-2010 Paul Ramsey <pramsey@opengeo.org>
# Copyright (C) 2005 Refractions Research Inc.
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

sub remove_line_break
{
    my $line = shift;
    $line =~ s/\s+/ /g;      # Replace all whitespace (including newlines, tabs) with single spaces
    $line =~ s/^\s+|\s+$//g; # Trim leading and trailing whitespace
    return $line;
}

#
# Commandline argument handling
#
($#ARGV == 0)
  ||die
"Usage: perl create_upgrade.pl <create_script.sql> [<schema>]\nCreates a new SQL script to upgrade all of the PostGIS functions.\n"
  if ( @ARGV < 1 || @ARGV > 3 );

my $sql_file = $ARGV[0];
my $module = 'postgis';
my $soname = '';
my $version_to = "";
my $version_to_full = "";
my $version_to_num = 0;
my $schema = "";
$schema = $ARGV[1] if @ARGV > 1;

my @renamed_deprecated_functions = ();

die "Unable to open input SQL file $sql_file\n"
  if ( !-f $sql_file );

#
# Search the SQL file for the target version number (the
# version we are upgrading *to*.
#
open( INPUT, $sql_file ) || die "Couldn't open file: $sql_file\n";
while(<INPUT>)
{
    #
    # Since 1.1.0 scripts/lib/release versions are the same
    #
    if (/INSTALL VERSION: (.*)/)
    {
        $version_to = $1;

        #last;
    }
    elsif (/TYPE raster/)
    {
        $module = 'postgis_raster';
    }
    elsif (m@('\$libdir/[^']*')@)
    {
        $soname = $1;
    }
}
close(INPUT);

die "Unable to locate target new version number in $sql_file\n"
  if( !$version_to );

if ( $version_to =~ /(\d+)\.(\d+)\.(\d+)([^' ]*)/ )
{
    $version_to_full = $1 . '.' . $2 . '.' . $3 . $4;
    $version_to = $1 . "." . $2;
    $version_to_num = 100 * $1 + $2;
}
else
{
    die "Version to number invalid, must be of form X.X.X\n";
}

print qq{
--
-- UPGRADE SCRIPT TO PostGIS $version_to_full
--

};

print "LOAD $soname;\n" if ($soname);

#print "BEGIN;\n";
print "SET search_path TO $schema;\n" if $schema;

#
# Add in the conditional check function to ensure this script is
# not being applied to a major version update.
#
while(<DATA>)
{
    s/NEWVERSION/$version_to_full/g;
    s/MODULE/$module/g;
    print;
}

#
# Go through the SQL file and strip out objects that cannot be
# applied to an existing, loaded database: types and operators
# and operator classes that have already been defined.
#
my $comment = '';
open( INPUT, $sql_file ) || die "Couldn't open file: $sql_file\n";
while(<INPUT>)
{

    if (/^\-\-/)
    {
        $comment .= $_;
        next;
    }

    #
    # Allow through deprecations from postgis_drop.sql
    #
    print if (/^drop function /i);
    print if (/^drop aggregate /i);

    if (/^create domain\s+([^.]+)\.([^\s]+)\s+as\s+([^;]+)/i) {
        my $schema = lc($1);
        my $name   = lc($2);
        my $type   = lc($3);
        $type = remove_line_break($type); # Normalize whitespace using helper function

        my $def .= $_;
        my $subcomment = '';
        my @constraints; # [type, definition, last_updated, comment]
        my $type_changed = 0;
        my $all_constraints_created_together = 1;

        print "\n\n-- Domain: $schema.$name($type)\n";

        my @replaced_array = parse_replaces($comment);

        my $domain_last_updated = parse_last_updated($comment);
        if ( !$domain_last_updated )
        {
            die "ERROR: no last updated info for domain  '${schema}.${name}($type)'\n";
        }
        my $missing = parse_missing($comment);

        unless (/;\s*$/) {
            while(my $line = <INPUT>) {
                if ($line =~ /^\s*\-\-/) {
                    $subcomment .= remove_line_break($line);
                    next;
                }

                # Find NOT NULL or NULL constraint in domain definition ignore ;
                if ($line =~ /^\s*(NOT\s+NULL|NULL)\s*;?\s*$/i) {
                    my $ctype = uc($1);

                    my $last_updated = parse_last_updated($subcomment);
                    my $missing = parse_missing($subcomment);

                    if ( $last_updated ) {
                        push @constraints, [$ctype, '', $ctype, $last_updated, $missing, $subcomment];
                    } else {
                        die "ERROR: no last updated info for constraint '${ctype}' in ${schema}.${name}($type)\n";
                    }

                    $subcomment = '';

                    if ($line =~ /;\s*$/) {
                        $def .= "$ctype;\n";
                        last;
                    } else {
                        $def .= "$ctype\n";
                    }

                    next;
                }

                if ($line =~ /CONSTRAINT\s+(\w+)\s+CHECK/i) {
                    my $constraint_name = $1;
                    my $constraint = $line;
                    my $open_parens = ($constraint =~ tr/(//);
                    my $close_parens = ($constraint =~ tr/)//);
                    my $found_semicolon = 0;

                    while ($open_parens == 0 || $open_parens > $close_parens) {
                        my $nextline = <INPUT>;
                        last unless defined $nextline;

                        if ($nextline =~ /;\s*$/) {
                            $found_semicolon = 1;
                            $nextline =~ s/;\s*$//;
                        }

                        $constraint .= $nextline;
                        $open_parens += ($nextline =~ tr/(//);
                        $close_parens += ($nextline =~ tr/)//);
                    }

                    # Make a single line of the constraint
                    $constraint = remove_line_break($constraint);

                    my $last_updated = parse_last_updated($subcomment);
                    my $missing = parse_missing($subcomment);

                    if ( $last_updated ) {
                        push @constraints, ['CHECK', $constraint_name, $constraint, $last_updated, $missing, $subcomment];
                    } else {
                        die "ERROR: no last updated info for constraint '${constraint_name}' in ${schema}.${name}($type)\n";
                    }

                    $subcomment = ''; # Reset subcomment after using it

                    if ($found_semicolon) {
                        $def .= "$constraint;\n";
                        last;
                    } else {
                        $def .= "$constraint\n";
                    }

                    next;
                }

                last if $line =~ /;\s*$/;  # End of domain definition
            }
        }

        foreach my $c (@constraints) {
            my ($ctype, $cname, $cdef, $last_updated, $missing, $comment) = @$c;

            if ($domain_last_updated != $last_updated) {
                $all_constraints_created_together = 0;
            }
        }

        if (@replaced_array) {
            foreach my $replaced(@replaced_array)
            {
                my ($rname, $rargs, $ver) = @$replaced;
                $rname = lc($rname); # lowercase the name
                my $old_type = lc($rargs);
                my $new_type = lc($type);

                if ($old_type ne $new_type)
                {
                    $type_changed = 1;
                    print <<"EOF";
-- ${schema}.${name}($old_type) -- LastUpdated: ${domain_last_updated}
-- Updated Domain ${schema}.${name}($new_type)
-- We cannot drop the old domain, so we modify it
DO LANGUAGE 'plpgsql'
\$postgis_domain_upgrade\$
BEGIN
IF $ver > version_from_num
EOF
                    print "OR version_from_num IN ( ${missing} )" if ($missing);
                    print <<"EOF";
    FROM _postgis_upgrade_info()
THEN
    PERFORM _postgis_topology_upgrade_domain_type('${name}', '${old_type}', '${new_type}', '${ver}');
END IF;
END
\$postgis_domain_upgrade\$;\n
EOF
                    foreach my $c (@constraints) {
                        my ($ctype, $cname, $cdef, $last_updated, $missing, $comment) = @$c;

                        print <<"EOF";
ALTER DOMAIN ${schema}.${name} DROP CONSTRAINT IF EXISTS $cname;
ALTER DOMAIN ${schema}.${name} ADD $cdef;\n
EOF
                    }

                }
            }
        } elsif ($all_constraints_created_together) {
            print <<"EOF";
DO LANGUAGE 'plpgsql'
\$postgis_domain_upgrade\$
BEGIN
IF $domain_last_updated > version_from_num
EOF
            print "OR version_from_num IN ( ${missing} )\n" if ($missing);
            print <<"EOF";
FROM _postgis_upgrade_info()
THEN
    EXECUTE \$postgis_domain_upgrade_parsed_def\$ $def \$postgis_domain_upgrade_parsed_def\$;
END IF;
END
\$postgis_domain_upgrade\$;
EOF
        } else {
            foreach my $c (@constraints) {
                my ($ctype, $cname, $cdef, $last_updated, $missing, $comment) = @$c;

                print <<"EOF";
DO LANGUAGE 'plpgsql'
\$postgis_domain_upgrade\$
BEGIN
IF $last_updated > version_from_num
EOF
            print "OR version_from_num IN ( ${missing} )\n" if ($missing);
            print <<"EOF";
FROM _postgis_upgrade_info()
THEN
    EXECUTE \$postgis_domain_upgrade_parsed_def\$ ALTER DOMAIN ${schema}.${name} DROP CONSTRAINT IF EXISTS $cname \$postgis_domain_upgrade_parsed_def\$;
    EXECUTE \$postgis_domain_upgrade_parsed_def\$ ALTER DOMAIN ${schema}.${name} ADD $cdef \$postgis_domain_upgrade_parsed_def\$;
END IF;
END
\$postgis_domain_upgrade\$;
EOF
                print "\n";
            }
        }
    }

    if (/^alter\s+domain\s+([^.]+)\.([^\s]+)\s/ims)
    {
        # This is a domain alteration, which we do not support in upgrade scripts
        # since we cannot drop and recreate domains.
        # We will just die with an error message.
        my $schema = $1;
        my $name   = remove_line_break(lc($2));
        #print "Altering domain $schema.$name is not supported in upgrade scripts.\n";
        # Die if we find an ALTER DOMAIN command
        die "ERROR: ALTER DOMAIN command found for domain '${name}'\n";
    }

    if (/^create or replace function/i)
    {
        my $def .= $_;
        my @replaced_array = parse_replaces($comment);
        my $endfunc = 0;
        while(<INPUT>)
        {
            $def .= $_;
            $endfunc = 1 if /^\s*(\$[^\$]*\$\s*)?LANGUAGE /;
            last if ( $endfunc && /\;/ );
        }
        foreach my $replaced (@replaced_array)
        {
            my ($name, $args, $ver) = @$replaced;
            $name = lc($name); # lowercase the name

            # Check if there are argument names
            my @argtypearray;
            my @argnamearray;
            my $numnamedargs = 0;
            foreach my $a ( split ',', $args )
            {
                my $argtype = $a;

                # NOTE: we should not consider OUT parameters
                #print "-- ARG: [$argtype]\n";
                if ( $argtype =~ / *([^ ]+)  *([^ ]+)/ ) {
                    my $argname = $1;
                    $argtype = $2;
                    #print "-- ARGNAME: [$argname]\n";
                    #print "-- ARGTYPE: [$argtype]\n";
                    push @argnamearray, "'$argname'";
                    $numnamedargs++;
                }

                push @argtypearray, "$argtype";
            }
            my $argnames = join ',', @argnamearray;
            my $argtypes = join ',', @argtypearray;

            my $renamed_suffix =  '_deprecated_by_postgis_' . ${ver};
            my $renamed_suffix_len = length(${renamed_suffix});
            my $renamed = substr($name, 0, (63-${renamed_suffix_len})) . ${renamed_suffix};
            my $replacement = "${renamed}(${args})";
            push @renamed_deprecated_functions, ${renamed};
            print <<"EOF";
-- Rename $name ( $args ) deprecated in PostGIS $ver, if needed
DO LANGUAGE 'plpgsql'
\$postgis_proc_upgrade\$
DECLARE
    detail TEXT;
    argnames TEXT[];
BEGIN

    -- Check if the deprecated function exists
    BEGIN

        SELECT proargnames
        FROM pg_catalog.pg_proc
        WHERE oid = '$name($argtypes)'::regprocedure
        INTO argnames;

EOF
            # Check for argument names match, if any argument name
            # was given in the Replaces comment
            if ( $numnamedargs )
            {
                print <<"EOF";
        -- Check if the deprecated function has the expected $numnamedargs argument names
        IF argnames[1:$numnamedargs] != ARRAY[$argnames]::text[]
        THEN
            RAISE DEBUG
                'Function $name($argtypes) exist but has argnames % (not %)',
                argnames, ARRAY[$argnames];
            RETURN; -- nothing to do
        END IF;
EOF
            }
            print <<"EOF";

    EXCEPTION
    WHEN undefined_function THEN
        RAISE DEBUG 'Replaced function $name($argtypes) does not exist';
        RETURN; -- nothing to do
    WHEN OTHERS THEN
        GET STACKED DIAGNOSTICS detail := PG_EXCEPTION_DETAIL;
        RAISE EXCEPTION 'Checking if replaced function $name($args) exists got % (%)', SQLERRM, SQLSTATE
            USING DETAIL = detail;
    END;

    -- Rename the replaced function, to avoid ambiguities.
    -- The renamed function will eventually be drop.
    BEGIN
        ALTER FUNCTION $name( $args ) RENAME TO ${renamed};
    EXCEPTION
    WHEN undefined_function THEN
        RAISE DEBUG 'Replaced function $name($args) does not exist';
    WHEN OTHERS THEN
        GET STACKED DIAGNOSTICS detail := PG_EXCEPTION_DETAIL;
        RAISE EXCEPTION 'Attempting to rename replaced function $name($args) got % (%)', SQLERRM, SQLSTATE
            USING DETAIL = detail;
    END;

END;
\$postgis_proc_upgrade\$;
EOF
        }

        # TODO: set owner to current owner, in case the new signature exists

        print $def;
    }

    if (/^create type ([\w.]+)/i)
    {
        my $newtype = $1;
        my $def .= $_;
        my @replaced_array = parse_replaces($comment);

        my @attributes;
        my $attr_comment = '';
        while(<INPUT>) {
            # End of type definition
            last if /^\s*\)\s*;?\s*$/;

            # Skip empty lines
            next if /^\s*$/;

            # Handle comment-only lines (attribute doc lines)
            if (/^\s*--(.*)$/) {
                $attr_comment .= ($attr_comment ? "\n" : "") . $1;
                next;
            }

            # Parse attribute line, possibly with inline comment
            if (/^\s*([\w"]+)\s+([\w\[\]]+)\s*(--.*)?[,]?\s*$/) {
                my ($attr_name, $attr_type, $inline_comment) = ($1, $2, $3);
                $attr_name =~ s/^"//; $attr_name =~ s/"$//; # remove quotes if any
                my $full_comment = $attr_comment;
                $full_comment .= ($full_comment && $inline_comment ? "\n" : "") if $inline_comment;
                $full_comment .= $inline_comment ? $inline_comment =~ s/^\s*--\s*//r : '';
                push @attributes, {
                    name    => remove_line_break($attr_name),
                    type    => remove_line_break($attr_type),
                    comment => $full_comment,
                };
                $attr_comment = '';
            }
        }

        my $last_updated = parse_last_updated($comment);
        if ( !$last_updated )
        {
            die "ERROR: no last updated info for type '${newtype}'\n";
        }
        my $missing = parse_missing($comment);

        if (@replaced_array)
        {
            my ($name, $oldargtypes, $version) = @{$replaced_array[0]};
            my @args = split ',', $oldargtypes;

            # Compare with new @attributes
            my $new_count = scalar @attributes;
            my $old_count = scalar @args;

            if ($new_count != $old_count) {
                die "ERROR: Type ${newtype} has ${new_count} attributes, but ${old_count} were replaced.\n";
            }

            for (my $i = 0; $i < @attributes; $i++) {
                my $attr = $attributes[$i];
                my $attr_name = $attr->{name};
                my $new_type = $attr->{type};
                my $old_type = remove_line_break($args[$i]);

                if ($old_type ne $new_type) {
                    print "-- Alter Type Attribute from ${old_type} to ${new_type} -- LastUpdated: ${last_updated}\n";
                    print "SELECT _postgis_topology_upgrade_user_type_attribute('${name}', '${attr_name}', '${old_type}', '${new_type}', '${version}');\n\n"
                }
            }
        } else {
            print "-- Type ${newtype} -- LastUpdated: ${last_updated}\n";
            print <<"EOF";
DO LANGUAGE 'plpgsql'
\$postgis_type_upgrade\$
BEGIN
  IF $last_updated > version_from_num
EOF
        print "OR version_from_num IN ( ${missing} )" if ($missing);
        print <<"EOF";
     FROM _postgis_upgrade_info()
  THEN
      EXECUTE \$postgis_type_upgrade_parsed_def\$ $def \$postgis_type_upgrade_parsed_def\$;
  END IF;
END
\$postgis_type_upgrade\$;
EOF
        }
        print "\n\n";
    }

    if (/^do *language .*\$\$/i)
    {
        print;
        while(<INPUT>)
        {
            print;
            last if /\$\$/;
        }
    }

    # This code handles casts by dropping and recreating them.
    if (/^create cast\s+\(\s*([^\s]+)\s+as\s+([^)]+)\)/i)
    {
        my $type1 = $1;
        my $type2 = $2;
        my $def = $_;
        unless (/;$/)
        {
            while(<INPUT>)
            {
                $def .= $_;
                last if /;$/;
            }
        }
        print "DROP CAST IF EXISTS ($type1 AS $type2);\n";
        print $def;
    }

    # This code handles aggregates by dropping and recreating them.
    # For PG12 use REPLACE instead
    if (/^create aggregate\s+([^(]+)\s*\(/i)
    {
        my $aggname = $1;

        #print "-- Aggname ${aggname}\n";
        my $aggtype = 'unknown';
        my $def = $_;
        if (/^create aggregate\s+\S+\s*\(([^)]*)\)/i)
        {
            $aggtype = $1;
            $aggtype =~ s/\s*,\s*/,/g; # drop spaces around commas
            $aggtype =~ s/\s\s*/ /g; # collapse multiple spaces into one
        }
        while(<INPUT>)
        {
            $def .= $_;
            $aggtype = $1 if (/basetype\s*=\s*([^,]*)\s*,/i);
            last if /\);/;
        }
        my $aggsig = "$aggname($aggtype)";

        #print "-- Checking comment $comment\n";
        my $last_updated = parse_last_updated($comment);
        if ( !$last_updated )
        {
            die "ERROR: no last updated info for aggregate '${aggsig}'\n";
        }

        my $pg12_def = $def;
        $pg12_def =~ s/CREATE AGGREGATE/CREATE OR REPLACE AGGREGATE/;
        if ($pg12_def eq "")
        {
            $pg12_def = "RAISE EXCEPTION 'Could not parse AGGREGATE'";
        }
        print "-- Aggregate ${aggsig} -- LastUpdated: ${last_updated}\n";
        print <<"EOF";
DO LANGUAGE 'plpgsql'
\$postgis_proc_upgrade\$
BEGIN
  IF pg_catalog.current_setting('server_version_num')::integer >= 120000
  THEN
    EXECUTE \$postgis_proc_upgrade_parsed_def\$ $pg12_def \$postgis_proc_upgrade_parsed_def\$;
  ELSIF $last_updated > version_from_num OR (
      $last_updated = version_from_num AND version_from_isdev
    ) FROM _postgis_upgrade_info()
  THEN
    EXECUTE 'DROP AGGREGATE IF EXISTS $aggsig';
    EXECUTE \$postgis_proc_upgrade_parsed_def\$ $def \$postgis_proc_upgrade_parsed_def\$;
  END IF;
END
\$postgis_proc_upgrade\$;
EOF
    }

    # This code handles operators by creating them if needed
    if (/^create operator\s+(\S+)\s*\(/i)
    {
        my $opname = $1;
        my $opleft = 'unknown';
        my $opright = 'unknown';
        my $def = $_;
        my $subcomment = '';
        my @subobjects; # minversion, attribute, value
        while(<INPUT>)
        {
            if (/^\s*\-\-/)
            {
                $subcomment .= $_;
                next;
            }

            $def .= $_;
            $opleft = $1 if (/leftarg\s*=\s*(\w+)\s*,/i);
            $opright = $1 if (/rightarg\s*=\s*(\w+)\s*,/i);

            # Support changing restrict selectivity at later versions
            if (/\s+(RESTRICT|JOIN)\s*=\s*([^,\n]+)/)
            {
                my $last_updated = parse_last_updated($subcomment);
                if ($last_updated)
                {
                    push @subobjects, [$last_updated, $1, $2];
                }
            }

            last if /\);/;
        }
        my $opsig = $opleft . " " . $opname . " " . $opright;

        my $last_updated = parse_last_updated($comment);
        if ( !$last_updated )
        {
            die "WARNING: no last updated info for operator '${opsig}'\n";
        }
        print "-- Operator ${opsig} -- LastUpdated: ${last_updated}\n";
        print <<"EOF";
DO LANGUAGE 'plpgsql'
\$postgis_proc_upgrade\$
BEGIN
    IF NOT EXISTS (
        SELECT o.oprname
        FROM
            pg_catalog.pg_operator o,
            pg_catalog.pg_type tl,
            pg_catalog.pg_type tr
        WHERE
            o.oprleft = tl.oid AND
            o.oprright = tr.oid AND
            o.oprcode != 0 AND
            o.oprname = '$opname' AND
            tl.typname = '$opleft' AND
            tr.typname = '$opright'
    )
    THEN
$def
EOF

        my $ELSE="ELSE -- version_from >= $last_updated";
        for my $subobj (@subobjects)
        {
            my $last_updated = @{$subobj}[0];
            my $attr = @{$subobj}[1];
            my $val = @{$subobj}[2];
            print <<"EOF";
    ${ELSE}
    -- Last Updated: ${last_updated}
    IF $last_updated > version_from_num FROM _postgis_upgrade_info() THEN
        ALTER OPERATOR ${opname} ( $opleft, $opright ) SET ( ${attr} = ${val} );
    END IF;
EOF
            $ELSE="";
        }

        print <<"EOF";
  END IF; -- version_from >= $last_updated
END
\$postgis_proc_upgrade\$;
EOF

    }

    # Always output create or replace view (see ticket #1097)
    if (/^create or replace view\s+(\S+)\s*/i)
    {
        print;
        while(<INPUT>)
        {
            print;
            last if /\;\s*$/;
        }
    }

    # Always output grant permissions (see ticket #3680)
    if (/^grant select\s+(\S+)\s*/i)
    {
        print;
        if ( !/\;\s*$/)
        {
            while(<INPUT>)
            {
                print;
                last if /\;\s*$/;
            }
        }
    }

    # Always output create or replace rule
    if (/^create or replace rule\s+(\S+)\s*/i)
    {
        print;
        while(<INPUT>)
        {
            print;
            last if /\;\s*$/;
        }
    }

    # This code handles operator family by creating them if we are doing a major upgrade
    if (/^create operator family\s+(\w+)\s+USING\s+(\w+)\s*/i)
    {
        my $opfname = $1;
        my $amname = $2;
        my $def = $_;
        my $opfsig = $opfname . " " . $amname;
        while(<INPUT>)
        {
            $def .= $_;
            last if /\);/;
        }

        my $last_updated = parse_last_updated($comment);
        if ( !$last_updated )
        {
            die "WARNING: no last updated info for operator family '${opfname}'\n";
        }
        print "-- Operator family ${opfsig} -- LastUpdated: ${last_updated}\n";
        print <<"EOF";
DO LANGUAGE 'plpgsql'
\$postgis_proc_upgrade\$
BEGIN
  IF $last_updated > version_from_num FROM _postgis_upgrade_info() THEN
    EXECUTE \$postgis_proc_upgrade_parsed_def\$ $def \$postgis_proc_upgrade_parsed_def\$;
  END IF;
END
\$postgis_proc_upgrade\$;
EOF
    }

    # This code handles operator classes by creating them if we are doing a major upgrade
    if (/^create operator class\s+(\w+)\s*/i)
    {
        my $opclassname = $1;
        my $opctype = 'unknown';
        my $opcidx = 'unknown';
        my $def = $_;
        my $last_updated;
        my $subcomment = '';
        my @subobjects; # minversion, definition
        while(<INPUT>)
        {
            if (/^\s*\-\-/)
            {
                $subcomment .= $_;
                next;
            }

            $def .= $_;
            $opctype = $1 if (/for type (\w+) /i);
            $opcidx = $1 if (/using (\w+) /i);

            # Support adding members at later versions
            if (/\s+(OPERATOR|FUNCTION)\s+[0-9]+\s+ /)
            {
                my $last_updated = parse_last_updated($subcomment);
                if ($last_updated)
                {
                    my $subdefn = $_;
                    chop $subdefn;
                    $subdefn =~ s/[,;]$//; # strip ending comma or semicolon
                     # argument types must be specified in ALTER OPERATOR FAMILY
                    if ( $subdefn =~ m/\s+(OPERATOR.*)(FOR.*)/ )
                    {
                        $subdefn = $1.'('.$opctype.','.$opctype.') '.$2;
                    }
                    elsif ( $subdefn =~ m/\s+(OPERATOR.*)/ )
                    {
                        $subdefn = $1.'('.$opctype.','.$opctype.') ';
                    }
                    elsif ( $subdefn =~ m/\s+(FUNCTION\s+[0-9]+ )(.*)/ )
                    {
                        $subdefn = $1.'('.$opctype.','.$opctype.') '.$2;
                    }
                    push @subobjects, [$last_updated, $subdefn];
                }
                $subcomment = '';
            }
            last if /;$/;
        }
        $opctype =~ tr/A-Z/a-z/;
        $opcidx =~ tr/A-Z/a-z/;

        $last_updated = parse_last_updated($comment);
        if ( !$last_updated )
        {
            die "WARNING: no last updated info for operator class '${opclassname}'\n";
        }
        print "-- Operator class ${opclassname} -- LastUpdated: ${last_updated}\n";
        print <<"EOF";
DO LANGUAGE 'plpgsql'
\$postgis_proc_upgrade\$
BEGIN

  IF $last_updated > version_from_num FROM _postgis_upgrade_info()
  THEN
    EXECUTE \$postgis_proc_upgrade_parsed_def\$
    $def    \$postgis_proc_upgrade_parsed_def\$;
EOF
        my $ELSE="ELSE -- version_from >= $last_updated";
        for my $subobj (@subobjects)
        {
            $last_updated = @{$subobj}[0];
            $def = @{$subobj}[1];
            print <<"EOF";
  $ELSE
    -- Last Updated: ${last_updated}
    IF $last_updated > version_from_num FROM _postgis_upgrade_info() THEN
      EXECUTE \$postgis_proc_upgrade_parsed_def\$
        ALTER OPERATOR FAMILY ${opclassname} USING ${opcidx}
          ADD $def;
      \$postgis_proc_upgrade_parsed_def\$;
    END IF;
EOF
            $ELSE="";
        }
        print <<"EOF";
  END IF; -- version_from >= $last_updated
END
\$postgis_proc_upgrade\$;
EOF
    }

    $comment = '';
}

close(INPUT);

print "DROP FUNCTION _postgis_upgrade_info();\n";

#print "COMMIT;\n";

1;

__END__

DO $$
DECLARE
    old_scripts text;
    new_scripts text;
    old_ver_int int[];
    new_ver_int int[];
    old_maj text;
    new_maj text;
    postgis_upgrade_info RECORD;
    postgis_upgrade_info_func_code TEXT;
BEGIN

    old_scripts := MODULE_scripts_installed();
    new_scripts := 'NEWVERSION';

    BEGIN
        new_ver_int := pg_catalog.string_to_array(
            pg_catalog.regexp_replace(
                new_scripts,
                '[^\d.].*',
                ''
            ),
            '.'
        )::int[];
    EXCEPTION WHEN OTHERS THEN
        RAISE EXCEPTION 'Cannot parse new version % into integers', new_scripts;
    END;

    BEGIN
        old_ver_int := pg_catalog.string_to_array(
            pg_catalog.regexp_replace(
                old_scripts,
                '[^\d.].*',
                ''
            ),
            '.'
        )::int[];
    EXCEPTION WHEN OTHERS THEN
        RAISE EXCEPTION 'Cannot parse old version % into integers', old_scripts;
    END;

    -- Guard against downgrade
    IF new_ver_int < old_ver_int
    THEN
        RAISE EXCEPTION 'Downgrade of MODULE from version % to version % is forbidden', old_scripts, new_scripts;
    END IF;


    -- Check for hard-upgrade being required
    SELECT into old_maj pg_catalog.substring(old_scripts, 1, 1);
    SELECT into new_maj pg_catalog.substring(new_scripts, 1, 1);

    -- 2.x to 3.x was upgrade-compatible, see
    -- https://trac.osgeo.org/postgis/ticket/4170#comment:1
    IF new_maj = '3' AND old_maj = '2' THEN
        old_maj = '3'; -- let's pretend old major = new major
    END IF;

    IF old_maj != new_maj THEN
        RAISE EXCEPTION 'Upgrade of MODULE from version % to version % requires a dump/reload. See PostGIS manual for instructions', old_scripts, new_scripts;
    END IF;

    WITH versions AS (
      SELECT 'NEWVERSION'::text as upgraded,
      MODULE_scripts_installed() as installed
    ) SELECT
      upgraded as scripts_upgraded,
      installed as scripts_installed,
      pg_catalog.substring(upgraded, '([0-9]+)\.')::int * 100 +
      pg_catalog.substring(upgraded, '[0-9]+\.([0-9]+)(\.|$)')::int
        as version_to_num,
      pg_catalog.substring(installed, '([0-9]+)\.')::int * 100 +
      pg_catalog.substring(installed, '[0-9]+\.([0-9]+)(\.|$)')::int
        as version_from_num,
      installed ~ 'dev|alpha|beta'
        as version_from_isdev
      FROM versions INTO postgis_upgrade_info
    ;

    postgis_upgrade_info_func_code := pg_catalog.format($func_code$
        CREATE FUNCTION _postgis_upgrade_info(OUT scripts_upgraded TEXT,
                                              OUT scripts_installed TEXT,
                                              OUT version_to_num INT,
                                              OUT version_from_num INT,
                                              OUT version_from_isdev BOOLEAN)
        AS
        $postgis_upgrade_info$
        BEGIN
            scripts_upgraded := %L :: TEXT;
            scripts_installed := %L :: TEXT;
            version_to_num := %L :: INT;
            version_from_num := %L :: INT;
            version_from_isdev := %L :: BOOLEAN;
            RETURN;
        END
        $postgis_upgrade_info$ LANGUAGE 'plpgsql' IMMUTABLE;
        $func_code$,
        postgis_upgrade_info.scripts_upgraded,
        postgis_upgrade_info.scripts_installed,
        postgis_upgrade_info.version_to_num,
        postgis_upgrade_info.version_from_num,
        postgis_upgrade_info.version_from_isdev);
    RAISE DEBUG 'Creating function %', postgis_upgrade_info_func_code;
    EXECUTE postgis_upgrade_info_func_code;
END
$$
LANGUAGE 'plpgsql';


