#!/bin/sh
#
# $Id$
#
# Autoconf Boot-strapping Script
#

giveup()
{
        echo
        echo "  Something went wrong, giving up!"
        echo
        exit 1
}

OSTYPE=`uname -s`

#
# Look for aclocal under various names. Solaris uses 
# versioned names.
#
for aclocal in aclocal aclocal-1.10 aclocal-1.9; do
    ACLOCAL=`which $aclocal 2>/dev/null`
    if test -x "${ACLOCAL}"; then
        break;
    fi
done
if [ ! $ACLOCAL ]; then
    echo "Missing aclocal!"
    exit
fi

#
# Look for libtoolize under various names.
#
for libtoolize in glibtoolize libtoolize; do
    LIBTOOLIZE=`which $libtoolize 2>/dev/null`
    if test -x "${LIBTOOLIZE}"; then
        break;
    fi
done
if [ ! $LIBTOOLIZE ]; then
    echo "Missing libtoolize!"
    exit
fi

#
# Run the actual autotools routines.
# 
#echo "Running libtoolize --force --copy"
#$LIBTOOLIZE --force --copy
echo "Running aclocal -I macros"
$ACLOCAL -I macros || giveup
echo "Running autoconf"
autoconf || giveup

echo "======================================"
echo "Now you are ready to run './configure'"
echo "======================================"
