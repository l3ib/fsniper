AC_INIT([sniper], [svn], [andrew@andrewyates.net])

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

AC_CONFIG_FILES( [Makefile] )
AC_OUTPUT
