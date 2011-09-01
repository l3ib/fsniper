AC_PREREQ(2.59)
AC_INIT([fsniper], [git], [andrew@andrewyates.net])
AM_MAINTAINER_MODE
AC_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)
AC_ISC_POSIX
AC_PROG_RANLIB
AC_LANG([C])
AC_LANG_COMPILER_REQUIRE
AC_HEADER_STDC([])
AC_PROG_CC
AC_PROG_INSTALL

AC_CHECK_LIB(magic, magic_open, [], AC_MSG_ERROR([libmagic not found]))
AC_CHECK_LIB(pcre, pcre_version, [], AC_MSG_ERROR([libpcre not found]))

AC_ARG_ENABLE([debug],
AC_HELP_STRING([--enable-debug],[build a debug version [[default=no]]]),
	[DEBUG=$enableval], [DEBUG="no"])
if test "$DEBUG" = "yes"; then
	FLAGS="-g3 -ggdb -Wall -Wextra -pedantic"
else
	FLAGS=""
fi

AC_ARG_ENABLE([efence],
AC_HELP_STRING([--enable-efence],[build with electric fence [[default=no]]]),
  [EFENCE=$enableval], [EFENCE="no"])
if test "$EFENCE" = "yes"; then
	AC_CHECK_HEADER([efence.h],[],
		[AC_MSG_ERROR([efence.h required for electric fence support])])
  AC_DEFINE(USE_EFENCE, 1, [Define to enable electric fence])
	LIBS="-lefence $LIBS"
fi

AC_ARG_ENABLE([syslog],
AC_HELP_STRING([--disable-syslog],[build without syslog support [[default=yes]]]),
  [SYSLOG=$enableval], [SYSLOG="yes"])
if test "$SYSLOG" = "yes"; then
	AC_CHECK_HEADER([syslog.h],[],
		[AC_MSG_ERROR([syslog.h required for syslog support])])
    AC_DEFINE(USE_SYSLOG, 1, [Define to enable syslog support])
else
    AC_MSG_WARN([Syslog support will be disabled])
fi

AC_SUBST(FLAGS)

AC_CONFIG_FILES( Makefile src/Makefile )
AC_OUTPUT
