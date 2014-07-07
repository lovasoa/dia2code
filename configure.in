dnl Process this file with autoconf to produce a configure script.
AC_INIT(Makefile.am)

AC_CONFIG_HEADERS(config.h)

AM_INIT_AUTOMAKE(dia2code,0.8.7)

dnl Checks for programs.
AC_PROG_CC
AC_PROG_RANLIB
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
  XML_INCLUDES=`$XML_CONFIG --cflags`
  LIBXML_LIBS=`$XML_CONFIG --libs`
  AC_SUBST(XML_INCLUDES)
  LIBS="$LIBS $LIBXML_LIBS"
fi

dnl Checks for header files.

AC_CHECK_HEADERS(dlfcn.h)

AC_CHECK_LIB(dl, dlopen,[LDFLAGS="$LDFLAGS -rdynamic"; LIBS="$LIBS -ldl"; CPPFLAGS="$CPPFLAGS -DDSO"])

dnl Checks for typedefs, structures, and compiler characteristics.

dnl Checks for library functions.

AC_OUTPUT(Makefile dia2code/Makefile dia2code/docs/Makefile dia2code/docs/en/Makefile )
