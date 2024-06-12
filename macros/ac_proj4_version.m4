dnl **********************************************************************
dnl *
dnl * PostGIS - Spatial Types for PostgreSQL
dnl * http://postgis.net
dnl * Copyright 2008 Mark Cave-Ayland
dnl *
dnl * This is free software; you can redistribute and/or modify it under
dnl * the terms of the GNU General Public Licence. See the COPYING file.
dnl *
dnl **********************************************************************

dnl
dnl Extract the PROJ version number from proj.h
dnl
dnl Sets PROJ_VERSION
dnl

AC_DEFUN([AC_PROJ_VERSION], [

	CFLAGS_SAVE="$CFLAGS"
	LDFLAGS_SAVE="$LDFLAGS"
	CFLAGS="$PROJ_CPPFLAGS"
	LDFLAGS="$PROJ_LDLAGS"

  AC_MSG_CHECKING([PROJ version via compiled code])
  AC_RUN_IFELSE([
    AC_LANG_PROGRAM([
      #ifdef HAVE_STDINT_H
      #include <stdio.h>
      #endif
      #include "proj.h"
    ],[
      FILE *fp;
      fp = fopen("conftest.out", "w");
      fprintf(fp, "%d.%d.%d\n", PROJ_VERSION_MAJOR, PROJ_VERSION_MINOR, PROJ_VERSION_PATCH);
      fclose(fp);
    ])
  ],[
    dnl The program ran successfully
    PROJ_VERSION=`cat conftest.out`
  ],[
    AC_MSG_ERROR([Could not extract PROJ version via compiled >=6 test])
  ])
  AC_MSG_RESULT($PROJ_VERSION)

	CFLAGS="$CFLAGS_SAVE"
	LDFLAGS="$LDFLAGS_SAVE"
])

