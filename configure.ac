AC_PREREQ(2.59)
AC_INIT([sniper], [svn], [andrew@andrewyates.net])
AM_MAINTAINER_MODE
AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)
AC_ISC_POSIX
AC_PROG_RANLIB
AC_LANG([C])
AC_LANG_COMPILER_REQUIRE
AC_HEADER_STDC([])
AC_PROG_CC
AC_PROG_INSTALL

AC_ARG_ENABLE([debug],
AC_HELP_STRING([--enable-debug],[build a debug version [[default=no]]]),
	[DEBUG=$enableval], [DEBUG="no"])
if test "$DEBUG" = "yes"; then
	FLAGS="-g3 -ggdb -Wall -pedantic"
else
	FLAGS=""
fi
AC_SUBST(FLAGS)

AC_CONFIG_FILES( Makefile src/Makefile )
AC_OUTPUT
