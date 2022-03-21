dnl Selecting tool implementation based on compiler vendor

dnl Wrap Autoconf check for whether GNU compiler is used
AC_DEFUN([PG_LANG_COMPILER_GNU],
[_AC_LANG_COMPILER_GNU])

dnl Check if Clang compiler is used
AC_DEFUN([PG_LANG_COMPILER_CLANG],
[
  AC_CACHE_CHECK([whether we are using Clang],
    [ac_cv_[]_AC_LANG_ABBREV[]_compiler_clang],
    [_AC_COMPILE_IFELSE([AC_LANG_PROGRAM([], [[#ifndef __clang__
      trigger error
      #endif
      ]])],
      [pg_compiler_clang=yes],
      [pg_compiler_clang=no])
    ac_cv_[]_AC_LANG_ABBREV[]_compiler_clang=$pg_compiler_clang
    ])
  pg_compiler_clang=$ac_cv_[]_AC_LANG_ABBREV[]_compiler_clang
])

dnl Determine compiler vendor
AC_DEFUN([PG_COMPILER_VENDOR],
[
  PG_LANG_COMPILER_CLANG
  PG_LANG_COMPILER_GNU
  pg_compiler_vendor=
  if test "$pg_compiler_clang" = "yes"; then
    pg_compiler_vendor="clang"
  elif test "$ac_compiler_gnu" = "yes"; then
    pg_compiler_vendor="gnu"
  fi
])

dnl Select AR based on compiler
AC_DEFUN([PG_PROG_AR],
[
  PG_COMPILER_VENDOR
  case "$pg_compiler_vendor" in
    "clang") AC_CHECK_TOOLS(AR, [llvm-ar ar], :) ;;
    "gnu")   AC_CHECK_TOOLS(AR, [gcc-ar ar], :) ;;
    *)       AC_CHECK_TOOL(AR, ar, :) ;;
  esac
])

dnl Select RANLIB based on compiler
AC_DEFUN([PG_PROG_RANLIB],
[
  PG_COMPILER_VENDOR
  case "$pg_compiler_vendor" in
    "clang") AC_CHECK_TOOLS(RANLIB, [llvm-ranlib ranlib], :) ;;
    "gnu")   AC_CHECK_TOOLS(RANLIB, [gcc-ranlib ranlib], :) ;;
    *)       AC_CHECK_TOOL(RANLIB, ranlib, :) ;;
  esac
])
