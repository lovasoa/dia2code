dnl Process this file with autoconf to produce a configure script.
AC_PREREQ([2.68])

AC_CONFIG_HEADER(config.h)

AC_INIT(dia2code, [0.9.0])
AC_CONFIG_SRCDIR([dia2code/main.c])
AM_INIT_AUTOMAKE
AC_CONFIG_MACRO_DIR([m4])

LT_INIT

dnl Checks for programs.
AC_PROG_CC
AN_MAKEVAR([AR], [AC_PROG_AR])
AN_PROGRAM([ar], [AC_PROG_AR])
AC_DEFUN([AC_PROG_AR], [AC_CHECK_TOOL(AR, ar, :)])
AC_PROG_AR
dnl Checks for libraries.

AC_PATH_PROG(XML_CONFIG, xml2-config, no)
if test "$XML_CONFIG" = "no"; then
  AC_MSG_ERROR([Cannot determine configuration of libxml.
Perhaps you forgot to install the package libxml2-devel ?])
else
  AC_SUBST(XML_CFLAGS, `$XML_CONFIG --cflags`)
  AC_SUBST(XML_LIBS, `$XML_CONFIG --libs`)
fi

dnl Checks for header files.

AC_CHECK_HEADERS(dlfcn.h)

AC_CHECK_LIB(dl, dlopen,[LDFLAGS="$LDFLAGS -rdynamic"
             LIBS="$LIBS -ldl"
             AC_DEFINE(DSO, 1, [dlopen is supported])])

dnl Checks for typedefs, structures, and compiler characteristics.

dnl Checks for library functions.

AC_OUTPUT(Makefile dia2code/Makefile dia2code/docs/Makefile dia2code/docs/en/Makefile)
