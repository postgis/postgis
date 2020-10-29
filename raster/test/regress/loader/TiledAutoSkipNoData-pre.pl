my $TARGETFILE = $TEST . '.tif';

if ( ! -e $TARGETFILE ) {
	my $FILERASTER = dirname($TEST) . "/testraster2.tif";
	link ("$FILERASTER", "$TARGETFILE") ||
		die("Cannot link $FILERASTER to $TARGETFILE: $!");
}

1;
