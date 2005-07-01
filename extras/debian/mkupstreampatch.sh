#!/bin/sh

#run this script from the debian/rules clean section
#make a diff from the clean upstream postgis to this patched for debian use

if [ -z "$1" ]; then
	echo "ERROR: please supply mkupstreampatch.sh with the postgis version as parameter"
	exit 1
fi

VERSION=`echo $1 | awk '{sub("([^.0-9]+)", "+&"); print $0;}'`

CURRENTDIR="`pwd`"
DEBIANDIR="$CURRENTDIR/debian"
PATCHDIR="$DEBIANDIR/patches"
mkdir -p $PATCHDIR
PATCHFILE="$PATCHDIR/upstream.diff"
PATCHLEVEL="$PATCHDIR/patch.level"

ORIGDIR="$CURRENTDIR.orig"
if [ ! -d $ORIGDIR ]; then
	ORIGFILE="$CURRENTDIR/../postgis_$VERSION.orig.tar.gz"
	if [ -f $ORIGFILE ]; then
		echo "patching from file $ORIGFILE"
		OURTMPDIR="/tmp/postgis_$$"
		mkdir -p "$OURTMPDIR"
		cd "$OURTMPDIR"
		tar xzf "$ORIGFILE"
		DIRTIP=`ls | head -n 1`
		ORIGDIR="$OURTMPDIR/$DIRTIP" #tar file root
		ln -s "$CURRENTDIR" "$DIRTIP.debian" #link to our dir
		CURRENTDIR="$OURTMPDIR/$DIRTIP.debian"
	else
		echo "WARNING: there is no source of original postgis in known location"
		exit 0
	fi
else
	echo "patching from dir $ORIGDIR"
fi

echo "$ORIGDIR" | awk -F / '{print NF;}' > "$PATCHLEVEL"
LC_ALL=C TZ=UTC0 diff -Naur -x debian "$ORIGDIR" "$CURRENTDIR" > "$PATCHFILE"

if [ -n "$OURTMPDIR" ]; then
	rm -rf "$OURTMPDIR"
fi

exit 0
