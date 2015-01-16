# Process this file with autoconf to produce a configure script.
# $Id$

m4_define(libicecast_major, 2)
m4_define(libicecast_minor, 3)
m4_define(libicecast_micro, 1)
m4_define(libicecast_version, libicecast_major.libicecast_minor.libicecast_micro)

AC_INIT([libicecast], libicecast_version, [icecast-dev@xiph.org])
AC_PREREQ([2.54])
dnl AC_CONFIG_SRCDIR([src/shout.c])
AM_CONFIG_HEADER(config.h)
# config.h guard
AH_TOP([#ifndef __CONFIG_H__
#define __CONFIG_H__ 1])
AH_BOTTOM([#endif])

AC_DEFINE([LIBICECAST_MAJOR], libicecast_major, [Icecast library major version])
AC_DEFINE([LIBICECAST_MINOR], libicecast_minor, [Icecast library minor version])
AC_DEFINE([LIBICECAST_MICRO], libicecast_micro, [Icecast library patch version])

VERSION=libicecast_version

AM_INIT_AUTOMAKE([libicecast], libicecast_version)
AM_MAINTAINER_MODE

dnl create our name mangling macro
dnl the prefix must be hardwired because of AH limitations
AH_VERBATIM([_mangle], [
/* name mangling to protect code we share with other libraries */
#define _mangle(proc) _icecast_ ## proc
])

AC_PROG_CC
AM_PROG_LIBTOOL

dnl Set some options based on environment

dnl openbsd headers break when _XOPEN_SOURCE is defined but without it seems
dnl to be fine
case "$ac_cv_host" in
   *openbsd* | *solaris* | *irix*)
   ;;
   *) AC_DEFINE(_XOPEN_SOURCE, 600, [Define if you have POSIX and XPG specifications])
   ;;
esac
if test -z "$GCC"; then
        case $host in 
        *-*-irix*)
                DEBUG="-g -signed" 
                CFLAGS="-O2 -w -signed"
                PROFILE="-p -g3 -O2 -signed"
		;;
        sparc-sun-solaris*)
                DEBUG="-v -g"
                CFLAGS="-xO4 -fast -w -fsimple -native -xcg92"
                PROFILE="-v -xpg -g -xO4 -fast -native -fsimple -xcg92 -Dsuncc"
		;;
        *)
                DEBUG="-g"
                CFLAGS="-O"
                PROFILE="-g -p" 
		;;
        esac
else
        XIPH_CFLAGS="-Wall -ffast-math -fsigned-char"
        AC_DEFINE(_GNU_SOURCE, ,[Define if you have POSIX and GNU specifications])
        DEBUG="-g"
        PROFILE="-pg -g"
fi

dnl Checks for programs.

dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([strings.h sys/timeb.h])

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
XIPH_C99_INTTYPES

dnl Checks for library functions.
AC_CHECK_FUNCS([gettimeofday ftime])

dnl Module checks
XIPH_NET

ACX_PTHREAD([
    LIBS="$LIBS $PTHREAD_LIBS"
    XIPH_CFLAGS="$XIPH_CFLAGS $PTHREAD_CFLAGS $PTHREAD_CPPFLAGS"
    CC="$PTHREAD_CC"
    ])

dnl If pkgconfig is found, install a icecast.pc file.

AC_ARG_ENABLE([pkgconfig],
  AC_HELP_STRING([--disable-pkgconfig],[disable pkgconfig data files (auto)]),
  [dopkgconfig="$enableval"], [dopkgconfig="maybe"])
if test "$dopkgconfig" = "maybe"
then
  AC_CHECK_PROG([PKGCONFIG], [pkg-config], [yes], [no])
else
  AC_MSG_CHECKING([whether pkgconfig should be used])
  PKGCONFIG="$dopkgconfig"
  AC_MSG_RESULT([$PKGCONFIG])
fi
AM_CONDITIONAL([HAVE_PKGCONFIG], [test "$PKGCONFIG" != "no"])

# Collect flags for icecast.pc

# I hate myself for doing this.
save_prefix="$prefix"
if test "$prefix" = "NONE"
then
  prefix="$ac_default_prefix"
fi
eval icecast_includedir="$includedir"
prefix="$save_prefix"

ICECAST_VERSION="$VERSION"
ICECAST_CPPFLAGS="-I$icecast_includedir $VORBIS_CFLAGS $PTHREAD_CPPFLAGS"
ICECAST_CFLAGS="$PTHREAD_CFLAGS"
ICECAST_LIBS="-licecast"

XIPH_CLEAN_CCFLAGS([$ICECAST_CPPFLAGS], [ICECAST_CPPFLAGS])
XIPH_CLEAN_CCFLAGS([$ICECAST_CFLAGS], [ICECAST_CFLAGS])
XIPH_CLEAN_CCFLAGS([$PTHREAD_LIBS $LIBS], [ICECAST_LIBDEPS])
AC_SUBST(PTHREAD_CPPFLAGS)
AC_SUBST(ICECAST_LIBDEPS)
AC_SUBST(ICECAST_REQUIRES)
AC_SUBST(ICECAST_CPPFLAGS)
AC_SUBST(ICECAST_CFLAGS)

dnl Make substitutions

AC_SUBST(LIBTOOL_DEPS)
AC_SUBST(OPT)
AC_SUBST(LIBS)
AC_SUBST(DEBUG)
AC_SUBST(CFLAGS)
AC_SUBST(PROFILE)
AC_SUBST(XIPH_CFLAGS)
AC_SUBST(XIPH_CPPFLAGS)

AC_OUTPUT([Makefile
  net/Makefile timing/Makefile
  thread/Makefile avl/Makefile httpp/Makefile log/Makefile
  icecast.pc])
