#!/usr/bin/env perl
use strict;
use warnings;

use File::Path qw(make_path);
use MIME::Base64 qw(decode_base64);

my ($source, $target, $asset_dir) = @ARGV;
die "usage: $0 SOURCE TARGET ASSET_DIR\n" unless defined $asset_dir;

open my $in, '<:raw', $source or die "Cannot read $source: $!\n";
my $svg = do { local $/; <$in> };
close $in;

make_path($asset_dir);

my $index = 0;
$svg =~ s{((?:xlink:)?href)=(['"])data:image/(png|gif|jpe?g);base64,([^'"]+)\2}{
	my ($attribute, $quote, $extension, $encoded) = ($1, $2, $3, $4);
	$extension = 'jpg' if $extension eq 'jpeg';
	my $filename = sprintf('embedded-image-%03d.%s', ++$index, $extension);
	my $path = "$asset_dir/$filename";
	open my $out, '>:raw', $path or die "Cannot write $path: $!\n";
	print {$out} decode_base64($encoded);
	close $out;
	qq{$attribute=$quote$path$quote};
}gex;

open my $out, '>:raw', $target or die "Cannot write $target: $!\n";
print {$out} $svg;
close $out;
