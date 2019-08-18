dnl **********************************************************************
dnl *
dnl * PostGIS - Spatial Types for PostgreSQL
dnl * http://postgis.net
dnl * Copyright 2019 Paul Ramsey <pramsey@cleverelephant.ca>
dnl *
dnl * This is free software; you can redistribute and/or modify it under
dnl * the terms of the GNU General Public Licence. See the COPYING file.
dnl *
dnl **********************************************************************

dnl
dnl Return the protobuf-c version number
dnl https://github.com/protobuf-c/protobuf-c
dnl

dnl
dnl This function is only for use AFTER you have confirmed
dnl the presence of protobuf-c/protobuf-c.h
dnl

AC_DEFUN([AC_PROTOBUFC_VERSION], [

	AC_RUN_IFELSE([
		AC_LANG_PROGRAM([
			#ifdef HAVE_STDINT_H
			#include <stdio.h>
			#endif
			#include "protobuf-c/protobuf-c.h"
		],[
			FILE *fp = fopen("conftest.out", "w");
			fprintf(fp, "%d\n", PROTOBUF_C_VERSION_NUMBER);
			fclose(fp);
		])
	],[
		dnl The program ran successfully, so return the version number
		dnl in the form MAJOR * 1000000 + MINOR * 1000 + PATCH
		$1=`cat conftest.out`
	],[
		dnl The program failed so return a low version number
		$1="0"
	])

])

