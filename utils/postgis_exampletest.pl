#!/usr/bin/env perl

use strict;
use warnings;
use Getopt::Long;
use XML::LibXML;

my $mode;
my $help;

GetOptions(
	"report"       => sub { $mode = "report"; },
	"generate-sql" => sub { $mode = "generate-sql"; },
	"help"         => \$help,
) or usage(1);

usage(0) if $help;
usage(1) unless $mode;
my $xml_file = shift @ARGV or usage(1);
usage(1) if @ARGV;

my $parser = XML::LibXML->new();
$parser->recover(0);
$parser->no_network(1);
my $doc = $parser->parse_file($xml_file);

my $xpc = XML::LibXML::XPathContext->new($doc);
$xpc->registerNs("db", "http://docbook.org/ns/docbook");
$xpc->registerNs("xml", "http://www.w3.org/XML/1998/namespace");

if ($mode eq "report")
{
	print_report();
}
elsif ($mode eq "generate-sql")
{
	generate_sql();
}
else
{
	usage(1);
}

sub usage
{
	my ($exit_code) = @_;
	my $out = $exit_code ? *STDERR : *STDOUT;
	print $out "Usage: $0 --report|--generate-sql <postgis-out.xml>\n";
	exit $exit_code;
}

sub node_text
{
	my ($node) = @_;
	my $text = $node->textContent;
	$text =~ s/\r\n?/\n/g;
	return $text;
}

sub has_role
{
	my ($node, $role) = @_;
	my $roles = $node->getAttribute("role") // "";
	return grep { $_ eq $role } split /\s+/, $roles;
}

sub source_label
{
	my ($node) = @_;
	my ($refentry) = $xpc->findnodes("ancestor::db:refentry[1]", $node);
	my $id = $refentry ? ($refentry->getAttributeNS("http://www.w3.org/XML/1998/namespace", "id") // "") : "";
	$id ||= "line_" . ($node->line_number || "unknown");
	return $id . ":" . ($node->line_number || "unknown");
}

sub obvious_skip_reason
{
	my ($text) = @_;
	return "psql-meta" if $text =~ /^\s*\\\w+/m;
	return "external-context"
		if $text =~ /\b(sometable|gps_points|boston_polys|trajectories|fe_edges|mytable|nyc_|tiger\.|loader_|raster2pgsql|shp2pgsql|psql\b|wget\b|curl\b)\b/i;
	return "placeholder-output" if $text =~ /\.\.\./ || $text =~ /AD INFINITUM/i;
	return "notice-or-error" if $text =~ /\b(NOTICE|ERROR):/;
	return "";
}

sub looks_sql
{
	my ($text) = @_;
	return $text =~ /\b(SELECT|WITH|CREATE|INSERT|UPDATE|DELETE|ALTER|DROP)\b/i;
}

sub looks_query
{
	my ($text) = @_;
	return $text =~ /\b(SELECT|WITH)\b/i;
}

sub has_inband_expected
{
	my ($text) = @_;
	return 1 if $text =~ /--\s*(result|output|wkt collect)\s*--/i;
	return 1 if $text =~ /^\s*[a-zA-Z_][a-zA-Z0-9_]*\s*\n\s*-{3,}\s*\n/sm;
	return 1 if $text =~ /\(\d+ rows?\)/;
	return 0;
}

sub following_screen
{
	my ($node) = @_;
	my $next = $node->nextNonBlankSibling;
	$next = $next->nextNonBlankSibling
		if $next && $next->nodeType == XML_TEXT_NODE;
	return undef unless $next && $next->nodeType == XML_ELEMENT_NODE;
	return $next if $next->localname eq "screen";
	return undef;
}

sub parse_inband_example
{
	my ($text) = @_;
	my @lines = split /\n/, $text;

	for (my $i = 0; $i < @lines; $i++)
	{
		if ($lines[$i] =~ /--\s*(result|output|wkt collect)\s*--/i)
		{
			my $query = join("\n", @lines[0 .. $i - 1]);
			my @expected = grep { /\S/ } @lines[$i + 1 .. $#lines];
			return clean_example($query, \@expected);
		}
	}

	for (my $i = 0; $i + 1 < @lines; $i++)
	{
		next unless $lines[$i] =~ /^\s*([A-Za-z_][A-Za-z0-9_]*)\s*$/;
		next unless $lines[$i + 1] =~ /^\s*-{3,}\s*$/;

		my $query = join("\n", @lines[0 .. $i - 1]);
		my @expected;
		for (my $j = $i + 2; $j < @lines; $j++)
		{
			last if $lines[$j] =~ /^\s*$/;
			last if $lines[$j] =~ /^\(\d+ rows?\)\s*$/;
			push @expected, $lines[$j];
		}
		return clean_example($query, \@expected);
	}

	return;
}

sub parse_adjacent_example
{
	my ($query, $screen) = @_;
	my @lines = grep { /\S/ } split /\n/, $screen;
	return clean_example($query, \@lines);
}

sub clean_example
{
	my ($query, $expected_lines) = @_;
	$query =~ s/^\s+//;
	$query =~ s/\s+\z//;
	$query =~ s/;\s*\z//;

	my @expected;
	for my $line (@{$expected_lines})
	{
		$line =~ s/^\s+//;
		$line =~ s/\s+\z//;
		next if $line eq "";
		next if $line =~ /^\(\d+ rows?\)$/;
		push @expected, $line;
	}

	return unless $query =~ /\S/;
	return unless @expected;
	return ($query, \@expected);
}

sub opt_in_examples
{
	my @examples;
	for my $node ($xpc->findnodes("//db:programlisting[contains(concat(' ', normalize-space(\@role), ' '), ' example-test ')]"))
	{
		my $text = node_text($node);
		my ($query, $expected);
		if (has_inband_expected($text))
		{
			($query, $expected) = parse_inband_example($text);
		}
		else
		{
			my $screen = following_screen($node);
			($query, $expected) = parse_adjacent_example($text, node_text($screen)) if $screen;
		}

		push @examples, {
			label => source_label($node),
			query => $query,
			expected => $expected,
			valid => defined($query) && defined($expected),
		};
	}
	return @examples;
}

sub print_report
{
	my %stats = (
		programlisting_total => 0,
		sql_like => 0,
		select_or_with => 0,
		inband_expected => 0,
		psql_meta => 0,
		external_context => 0,
		placeholder_output => 0,
		notice_or_error => 0,
		cleanish_runnable_query => 0,
		obvious_not_runnable_query => 0,
		cleanish_inband_expected => 0,
		obvious_bad_inband_expected => 0,
		adjacent_programlisting_screen => 0,
		cleanish_adjacent_expected => 0,
		obvious_bad_adjacent_expected => 0,
	);

	for my $node ($xpc->findnodes("//db:programlisting"))
	{
		my $text = node_text($node);
		$stats{programlisting_total}++;
		$stats{sql_like}++ if looks_sql($text);
		next unless looks_query($text);
		$stats{select_or_with}++;

		my $reason = obvious_skip_reason($text);
		$stats{psql_meta}++ if $reason eq "psql-meta";
		$stats{external_context}++ if $reason eq "external-context";
		$stats{placeholder_output}++ if $reason eq "placeholder-output";
		$stats{notice_or_error}++ if $reason eq "notice-or-error";

		if ($reason) { $stats{obvious_not_runnable_query}++; }
		else { $stats{cleanish_runnable_query}++; }

		if (has_inband_expected($text))
		{
			$stats{inband_expected}++;
			if ($reason) { $stats{obvious_bad_inband_expected}++; }
			else { $stats{cleanish_inband_expected}++; }
		}

		my $screen = following_screen($node);
		if ($screen)
		{
			$stats{adjacent_programlisting_screen}++;
			my $screen_text = node_text($screen);
			my $bad_screen = $screen_text =~ /\.\.\.|NOTICE:|ERROR:|ST_AsText output|Packaging extension/i;
			if ($reason || $bad_screen) { $stats{obvious_bad_adjacent_expected}++; }
			else { $stats{cleanish_adjacent_expected}++; }
		}
	}

	my @examples = opt_in_examples();
	$stats{opt_in_example_tests} = scalar @examples;
	$stats{opt_in_parseable_example_tests} = scalar grep { $_->{valid} } @examples;

	for my $key (sort keys %stats)
	{
		print "$key=$stats{$key}\n";
	}
}

sub sql_quote
{
	my ($text) = @_;
	$text =~ s/'/''/g;
	return "'$text'";
}

sub sql_array
{
	my ($values) = @_;
	return "ARRAY[" . join(", ", map { sql_quote($_) } @{$values}) . "]::text[]";
}

sub generate_sql
{
	my @examples = opt_in_examples();

	print "-- Generated by utils/postgis_exampletest.pl. Do not edit by hand.\n";
	print "\\set ON_ERROR_STOP on\n";
	print "BEGIN;\n";
	print <<'SQL';
CREATE OR REPLACE FUNCTION pg_temp._postgis_exampletest_check(
	test_label text,
	test_query text,
	expected text[]
)
RETURNS void
LANGUAGE plpgsql
AS $$
DECLARE
	actual text[];
BEGIN
	EXECUTE format(
		'SELECT coalesce(array_agg(q.x::text), ARRAY[]::text[]) FROM (%s) AS q(x)',
		test_query
	) INTO actual;

	IF actual IS DISTINCT FROM expected THEN
		RAISE EXCEPTION E'Example test failed: %\nQuery: %\nExpected: %\nActual: %',
			test_label, test_query, expected, actual;
	END IF;
END;
$$;

SQL

	my $emitted = 0;
	for my $example (@examples)
	{
		die "Could not parse example-test at $example->{label}\n"
			unless $example->{valid};

		$emitted++;
		print "SELECT pg_temp._postgis_exampletest_check(\n";
		print "\t" . sql_quote($example->{label}) . ",\n";
		print "\t" . sql_quote($example->{query}) . ",\n";
		print "\t" . sql_array($example->{expected}) . "\n";
		print ");\n\n";
	}

	print "COMMIT;\n";
	print "\\echo 'manual example tests passed: $emitted'\n";
}
