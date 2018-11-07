AC_INIT([igloo], [0.0.1], [icecast@xiph.org])

AC_PREREQ([2.54])

AC_CONFIG_SRCDIR([httpp/httpp.c])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build-aux])

AC_PROG_CC

PKG_INSTALLDIR

AX_CHECK_COMPILE_FLAG([-std=c99], [
  AX_APPEND_FLAG([-std=c99])
], [
  AC_MSG_WARN([Compiler does not accept -std=c99 flag!])
])

AC_PROG_CC_C99

AS_IF([test "${ac_cv_prog_cc_c99}" = "no"], [
  AC_MSG_ERROR([No C99 compiler found!])
])

AX_APPEND_COMPILE_FLAGS([-Wall -Wextra -Wno-unused-parameter])

dnl With clang, we want an error for unknown flags instead of just warn
AX_APPEND_COMPILE_FLAGS([-Werror=unknown-warning-option -Werror=invalid-command-line-argument])

AC_SYS_LARGEFILE
AC_DEFINE([_GNU_SOURCE], 1, [Define to include GNU extensions to POSIX])

dnl Set build/host to default values
AC_CANONICAL_HOST
AC_CANONICAL_BUILD

dnl Check for which host we are compiling
AS_CASE("${host_os}",
  [linux*], [SYS="linux"],
  [darwin*], [SYS="darwin"],
  [SYS="${host_os}"]
)

AM_INIT_AUTOMAKE([tar-ustar foreign dist-zip subdir-objects])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AM_MAINTAINER_MODE([enable])

LT_INIT


dnl Sanitizer flags

AC_ARG_WITH([sanitizer],
    [AS_HELP_STRING([--with-sanitizer=(address/memory/undefined/thread)],
        [build with sanitizer flags (default disabled)])],
     [],
     [with_sanitizer=no])

AS_VAR_IF(with_sanitizer, no, [], [
    AX_CHECK_COMPILE_FLAG([-fsanitize=${with_sanitizer}], [
        AX_APPEND_FLAG([-fsanitize=${with_sanitizer}])
        AX_APPEND_FLAG([-fsanitize=${with_sanitizer}], [LDFLAGS])
    ], [
        AC_MSG_ERROR(["-fsanitize=${with_sanitizer} not supported!"])
    ])
    AX_APPEND_FLAG([-g])

    AS_CASE("${with_sanitizer}",
      [address], [
        AX_APPEND_COMPILE_FLAGS([-fsanitize-address-use-after-scope -fno-omit-frame-pointer -fsanitize=pointer-compare -fsanitize=pointer-subtract])
      ],
      [memory], [
        AX_APPEND_COMPILE_FLAGS([-fPIE -pie])
      ],
      [thread], [
        AX_APPEND_COMPILE_FLAGS([-fPIE -pie])
      ],
      []
    )
])

dnl Check for attributes
AX_GCC_TYPE_ATTRIBUTE([transparent_union])

dnl Checks for header files.
AC_HEADER_ASSERT
AC_HEADER_STDC
AC_HEADER_TIME

AC_CHECK_HEADERS([stdint.h inttypes.h], [
  ice_found_int_headers="yes";
  break;
])

AS_IF([test "$ice_found_int_headers" != "yes"], [
  AC_MSG_ERROR([Unable to find the standard integers headers])
])

AC_CHECK_HEADER([winsock2.h], [
  AC_DEFINE([HAVE_WINSOCK2_H], [1], [Define if you have winsock2.h on MINGW])
  LIBS="$LIBS -lwsock32"
])

AC_CHECK_HEADERS([sys/select.h sys/uio.h])
AC_CHECK_HEADERS([sys/types.h])
AC_CHECK_HEADERS([sys/timeb.h])
AC_CHECK_HEADERS([sys/socket.h])
AC_CHECK_HEADERS([pwd.h grp.h])

dnl Check for functions
AC_FUNC_FORK
AC_FUNC_CHOWN

AC_CHECK_FUNCS([gethostname])
AC_CHECK_FUNCS([gettimeofday ftime])

dnl Do not check for poll on Darwin, it is broken in some versions
AS_IF([test "${SYS}" != "darwin"], [
  AC_CHECK_FUNCS([poll])
])

AC_SEARCH_LIBS([nanosleep], [rt], [
  AC_DEFINE([HAVE_NANOSLEEP], [1], [Define if you have nanosleep])
])

dnl Checks for types and typedefs
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UID_T

dnl These tests are ordered based on solaris 8 tests
AC_SEARCH_LIBS([sethostent], [nsl], [
    AC_DEFINE([HAVE_SETHOSTENT], [1], [Define if you have the sethostent function])
])
AC_SEARCH_LIBS([getnameinfo], [socket], [
    AC_DEFINE([HAVE_GETNAMEINFO], [1], [Define if you have the getnameinfo function])
])
AC_CHECK_FUNCS([endhostent getaddrinfo inet_aton writev])


ACX_PTHREAD([], [AC_MSG_ERROR([POSIX threads missing])])

CFLAGS="${CFLAGS} ${PTHREAD_CFLAGS}"
CPPFLAGS="${CPPFLAGS} ${PTHREAD_CPPFLAGS}"
LIBS="${LIBS} ${PTHREAD_LIBS}"

AC_REQUIRE_AUX_FILE([tap-driver.sh])

AC_CONFIG_HEADERS([config.h])
AX_PREFIX_CONFIG_H([include/igloo/config.h], [igloo_CTC])

AC_CONFIG_FILES([
  Makefile
  avl/Makefile
  httpp/Makefile
  thread/Makefile
  log/Makefile
  net/Makefile
  timing/Makefile
  igloo.pc
])

AC_OUTPUT
