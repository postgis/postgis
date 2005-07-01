#!/bin/sh

LOG_FILE=debian/changelog
DEB_VER0="1"
#debian gis private version
#NEW_DEB_REL=`source debian/debian_versionname.sh`
#DEB_VER0="0.dgis.$NEW_DEB_REL.1"
LOG_TOP=`head -n1 $LOG_FILE` # first log line
LOG_FULL_VER=`echo $LOG_TOP | awk '{gsub("[)(]", "", $2); print $2;}'` # full ver
LOG_VER=`echo $LOG_FULL_VER | awk -F '-' '{print $1;}'` # upstream part
LOG_VER=`echo $LOG_VER | awk '{print toupper($0);}'`
LOG_DEB_VER=`echo $LOG_FULL_VER | awk -F '-' '{print $2;}'` # debian part
#LOG_DEB_REL=`echo $LOG_DEB_VER | awk -F . '{print $3;}'`
LOG_PKG=`echo $LOG_TOP | awk '{print $1;}'`
NEW_VER=`echo $1 | awk '{sub("([^.0-9]+)", "+&"); print $0;}'`
NEW_PKG=$2

INS_LINE1=
INS_LINE2=
INS_LINE3=

#PRE_VER="no"
#for VER_ARG in "CVS" "PRE" "RC"; do
#echo $NEW_VER | grep $VER_ARG
#    if [ -n "`echo $NEW_VER | grep $VER_ARG`" ]; then
#	    PRE_VER="yes"
#	fi
#done
#if [ ! "$PRE_VER" = "yes" ]; then
#    NEW_VER="${NEW_VER}RELEASE"
#fi

if [ -n "$NEW_VER" ] && [ ! "$NEW_VER" = "$LOG_VER" ]; then
	LOG_VER=$NEW_VER
	INS_LINE1='Automatic upstream version tracking.'
	LOG_FULL_VER="$LOG_VER-$DEB_VER0"
fi

if [ -n "$NEW_PKG" ] && [ ! "$NEW_PKG" = "$LOG_PKG" ]; then
	INS_LINE2='Making package alternative.'
	LOG_PKG=$NEW_PKG
fi

#if [ ! "$NEW_DEB_REL" = "$LOG_DEB_REL" ]; then
#	INS_LINE3="Making package for $NEW_DEB_REL."
#	DEB_VER="0.dgis.$NEW_DEB_REL."`echo $LOG_DEB_VER | awk -F . '{print $4;}'`
#	LOG_FULL_VER="$LOG_VER-$DEB_VER"
#fi

if [ -n "$INS_LINE1" ] || [ -n "$INS_LINE2" ] || [ -n "$INS_LINE3" ]; then 
	TMP_FILE=/tmp/changelog.$$
	cat $LOG_FILE > $TMP_FILE
	if [ -r /tmp/changelog.$$ ]; then
		echo "$LOG_PKG ($LOG_FULL_VER) "`echo $LOG_TOP | awk '{print $3" "$4;}'` > $LOG_FILE
		echo "" >> $LOG_FILE
		if [ ! -z "$INS_LINE1" ]; then
			echo "  * $INS_LINE1" >> $LOG_FILE
		fi
		if [ ! -z "$INS_LINE2" ]; then
			echo "  * $INS_LINE2" >> $LOG_FILE
		fi
		if [ ! -z "$INS_LINE3" ]; then
			echo "  * $INS_LINE3" >> $LOG_FILE
		fi
		echo "" >> $LOG_FILE
		echo " -- $DEBFULLNAME <$DEBEMAIL>  "`822-date` >> $LOG_FILE
		echo "" >> $LOG_FILE
		cat $TMP_FILE >> $LOG_FILE
	fi
	rm -f $TMP_FILE
fi
