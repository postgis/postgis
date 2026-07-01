use File::Basename qw(dirname);

sub make_tif {
	my ($name, $width, $xmin, $xmax, $ramp) = @_;
	my $dir = dirname($TEST);
	my $asc = "$dir/$name.asc";
	my $tif = "$dir/$name.tif";

	open(my $fh, '>', $asc) or die "Could not create $asc: $!";
	print $fh "ncols $width\n";
	print $fh "nrows 100\n";
	print $fh "xllcorner $xmin\n";
	print $fh "yllcorner -100\n";
	print $fh "cellsize 1\n";
	print $fh "NODATA_value -9999\n";
	for my $y (0..99) {
		my @vals = $ramp ? (1..$width) : ((1) x $width);
		print $fh join(' ', @vals), "\n";
	}
	close($fh);

	system(
		'gdal_translate', '-q', '-of', 'GTiff', '-ot', 'Byte',
		'-a_ullr', $xmin, '0', $xmax, '-100',
		$asc, $tif
	) == 0 or die "Could not create $tif";

	unlink $asc;
}

make_tif('OverviewAlignedGridA', 99, 0, 99, 0);
make_tif('OverviewAlignedGridB', 100, 99, 199, 1);
