#!/usr/bin/env perl
#
# Check invariants introduced by splitting documentation refentries into files.
#

use warnings;
use strict;
use File::Find;
use File::Spec;
use Getopt::Long;

my $docdir = "doc";
my $makefile;

GetOptions(
	"docdir=s" => \$docdir,
	"makefile=s" => \$makefile,
) || die "Usage: perl $0 [--docdir doc] [--makefile doc/Makefile.in]\n";

$makefile ||= File::Spec->catfile($docdir, "Makefile.in");

my %errors;

sub add_error {
	my ($group, $message) = @_;
	push @{$errors{$group}}, $message;
}

sub read_file {
	my ($path) = @_;
	open(my $fh, "<", $path) || die "Cannot open $path: $!\n";
	local $/;
	return <$fh>;
}

sub parse_makefile_list {
	my ($path, $variable) = @_;
	open(my $fh, "<", $path) || die "Cannot open $path: $!\n";
	my @items;
	my $collecting = 0;

	while (my $line = <$fh>) {
		chomp $line;
		if (!$collecting) {
			next unless $line =~ /^\Q$variable\E\s*=\s*(.*)$/;
			$collecting = 1;
			$line = $1;
		}
		elsif ($line ne "" && $line !~ /^\s/ && $line !~ /^\t/) {
			last;
		}

		$line =~ s/^\s+//;
		$line =~ s/\s+$//;
		next if $line eq "";
		$line =~ s/\\$//;
		$line =~ s/\s+$//;
		push @items, grep { $_ ne "" } split(/\s+/, $line);
	}

	close($fh);
	return @items;
}

sub is_split_source {
	my ($path) = @_;
	return $path =~ m{/}
		&& $path =~ /\.xml$/
		&& ($path =~ /^reference_/ || $path =~ /^extras_/);
}

sub doc_path {
	my ($relative) = @_;
	return File::Spec->catfile($docdir, split(m{/}, $relative));
}

sub relative_doc_path {
	my ($path) = @_;
	$path =~ s{^\Q$docdir\E/}{};
	return $path;
}

sub first_xml_tag {
	my ($text) = @_;
	pos($text) = 0;

	while (pos($text) < length($text)) {
		return (undef, "") unless $text =~ /\G\s*/gc;

		if ($text =~ /\G<!--.*?-->/sgc) {
			next;
		}
		if ($text =~ /\G<\?.*?\?>/sgc) {
			next;
		}
		if ($text =~ /\G<([A-Za-z_][\w.-]*:)?([A-Za-z_][\w.-]*)([^>]*)>/sgc) {
			return ($2, $&);
		}
		return (undef, "");
	}

	return (undef, "");
}

sub basename_without_xml {
	my ($path) = @_;
	$path =~ s{.*/}{};
	$path =~ s{\.xml$}{};
	return $path;
}

my @xml_sources = parse_makefile_list($makefile, "XML_SOURCES");
my %xml_source_set = map { $_ => 1 } @xml_sources;
my %source_counts;
$source_counts{$_}++ for @xml_sources;

for my $source (sort keys %source_counts) {
	add_error("XML_SOURCES", "$source is listed $source_counts{$source} times")
		if $source_counts{$source} > 1;
}

my @disk_split_sources;
find(
	{
		wanted => sub {
			return unless -f $_;
			return unless /\.xml$/;
			my $relative = relative_doc_path($File::Find::name);
			push @disk_split_sources, $relative if is_split_source($relative);
		},
		no_chdir => 1,
	},
	$docdir
);
@disk_split_sources = sort @disk_split_sources;

my %disk_split_set = map { $_ => 1 } @disk_split_sources;
my @listed_split_sources = sort grep { is_split_source($_) } @xml_sources;
my %listed_split_set = map { $_ => 1 } @listed_split_sources;

for my $source (sort grep { !$xml_source_set{$_} } @disk_split_sources) {
	add_error("XML_SOURCES", "$source exists on disk but is missing from XML_SOURCES");
}
for my $source (sort grep { !$disk_split_set{$_} } @listed_split_sources) {
	add_error("XML_SOURCES", "$source is listed in XML_SOURCES but does not exist");
}
for my $source (sort keys %xml_source_set) {
	add_error("XML_SOURCES", "$source is generated POT data, not source XML")
		if $source =~ /\.xml\.pot$/;
	if ($source =~ /\.xml$/ && ! -f doc_path($source)) {
		add_error("XML_SOURCES", "$source is listed in XML_SOURCES but does not exist");
	}
}

my $postgis_xml = File::Spec->catfile($docdir, "postgis.xml");
my $postgis_text = read_file($postgis_xml);
my (%entity_names, %entity_targets, %split_entities, %split_entity_by_target, %system_targets);

while ($postgis_text =~ /<!ENTITY\s+([^\s]+)\s+SYSTEM\s+"([^"]+)"\s*>/g) {
	my ($name, $target) = ($1, $2);
	$entity_names{$name}++;
	$entity_targets{$target}++;
	push @{$system_targets{$target}}, $name;
	if (is_split_source($target)) {
		$split_entities{$name} = $target;
		push @{$split_entity_by_target{$target}}, $name;
	}
}

for my $name (sort keys %entity_names) {
	add_error("postgis.xml entities", "entity $name is declared $entity_names{$name} times")
		if $entity_names{$name} > 1;
}
for my $target (sort keys %entity_targets) {
	next unless $entity_targets{$target} > 1 && is_split_source($target);
	add_error(
		"postgis.xml entities",
		"$target is declared by multiple entities: " . join(", ", @{$system_targets{$target}})
	);
}

for my $source (@listed_split_sources) {
	my @names = @{$split_entity_by_target{$source} || []};
	if (!@names) {
		add_error("postgis.xml entities", "$source is listed in XML_SOURCES but has no split entity");
	}
	elsif (@names > 1) {
		add_error("postgis.xml entities", "$source has multiple split entities: " . join(", ", @names));
	}
}

for my $name (sort keys %split_entities) {
	my $target = $split_entities{$name};
	add_error("postgis.xml entities", "entity $name points to $target, not listed in XML_SOURCES")
		unless $listed_split_set{$target};
	add_error("postgis.xml entities", "entity $name points to missing file $target")
		unless -f doc_path($target);

	my $expected = "ref_" . $target;
	$expected =~ s{\.xml$}{};
	$expected =~ s{/}{_}g;
	add_error("postgis.xml entities", "entity $name for $target should be named $expected")
		if $name ne $expected;
}

my %groups;
for my $source (@disk_split_sources, @listed_split_sources) {
	my ($group) = split(m{/}, $source, 2);
	$groups{$group} = 1;
}

my %entity_name_by_target;
for my $target (keys %split_entity_by_target) {
	my @names = @{$split_entity_by_target{$target}};
	$entity_name_by_target{$target} = $names[0] if @names == 1;
}

my %referenced_split_entities;
for my $group (sort keys %groups) {
	my $wrapper_source = "$group.xml";
	my $wrapper = doc_path($wrapper_source);
	my %group_entities = map {
		$split_entities{$_} =~ /^\Q$group\E\// ? ($_ => $split_entities{$_}) : ()
	} keys %split_entities;

	if (! -f $wrapper) {
		add_error("wrapper linkage", "$wrapper_source is missing for split directory $group");
		next;
	}
	add_error("wrapper linkage", "$wrapper_source exists but is missing from XML_SOURCES")
		unless $xml_source_set{$wrapper_source};

	my $wrapper_text = read_file($wrapper);
	my %ref_counts;
	while ($wrapper_text =~ /&([^;\s]+);/g) {
		my $ref = $1;
		next unless $ref =~ /^ref_/;
		$ref_counts{$ref}++;
		$referenced_split_entities{$ref} = 1;
	}

	for my $ref (sort keys %ref_counts) {
		add_error("wrapper linkage", "$wrapper_source references &$ref; $ref_counts{$ref} times")
			if $ref_counts{$ref} > 1;
		if (!exists $split_entities{$ref}) {
			add_error("wrapper linkage", "$wrapper_source references undeclared split entity &$ref;");
			next;
		}
		my ($target_group) = split(m{/}, $split_entities{$ref}, 2);
		add_error("wrapper linkage", "$wrapper_source references &$ref; from $target_group, not $group")
			if $target_group ne $group;
	}

	for my $name (sort { $group_entities{$a} cmp $group_entities{$b} } keys %group_entities) {
		add_error("wrapper linkage", "$group_entities{$name} is declared but not referenced by $wrapper_source")
			unless $ref_counts{$name};
	}

	add_error("wrapper linkage", "$wrapper_source still contains inline refentry markup")
		if $wrapper_text =~ /<([A-Za-z_][\w.-]*:)?refentry\b/;
}

for my $target (sort keys %entity_name_by_target) {
	my $name = $entity_name_by_target{$target};
	add_error("wrapper linkage", "$target is declared but unreachable from its wrapper")
		unless $referenced_split_entities{$name};
}

for my $source (@disk_split_sources) {
	my $text = read_file(doc_path($source));
	my ($root_tag, $opening_tag) = first_xml_tag($text);
	add_error("leaf XML shape", "$source root element is " . (defined $root_tag ? $root_tag : "unknown") . ", not refentry")
		if !defined $root_tag || $root_tag ne "refentry";
	if ($opening_tag !~ /\b(?:xml:)?id\s*=\s*"([^"]+)"/) {
		add_error("leaf XML shape", "$source refentry root has no xml:id");
	}
	else {
		my $xml_id = $1;
		my $expected_id = basename_without_xml($source);
		add_error("leaf XML shape", "$source has xml:id $xml_id, expected $expected_id")
			if exists $entity_name_by_target{$source} && $xml_id ne $expected_id;
	}
}

find(
	{
		wanted => sub {
			return unless -f $_;
			return unless /\.xml\.pot$/;
			my $relative = relative_doc_path($File::Find::name);
			return if $relative =~ m{^po/};
			(my $source = $relative) =~ s{\.pot$}{};
			add_error("generated POT", "$relative has no matching XML_SOURCES entry")
				unless $xml_source_set{$source};
			add_error("generated POT", "$relative has no matching source XML file")
				unless -f doc_path($source);
		},
		no_chdir => 1,
	},
	$docdir
);

if (%errors) {
	print "split documentation layout check failed:\n";
	for my $group (sort keys %errors) {
		print "$group:\n";
		for my $message (@{$errors{$group}}) {
			print "  - $message\n";
		}
	}
	exit 1;
}

print "split documentation layout is consistent\n";
exit 0;
