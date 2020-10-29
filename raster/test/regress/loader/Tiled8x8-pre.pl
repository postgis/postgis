my $TARGETFILE = $TEST . '.tif';

if ( ! -e $TARGETFILE ) {
	my $FILERASTER = dirname($TEST) . "/testraster.tif";
	link ("$FILERASTER", "$TARGETFILE") ||
		die("Cannot link $FILERASTER to $TARGETFILE: $!");
}

1;
