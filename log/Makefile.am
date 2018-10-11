## Process this with automake to create Makefile.in

AUTOMAKE_OPTIONS = foreign

EXTRA_DIST = test.c

noinst_LTLIBRARIES = libicelog.la
pkginclude_HEADERS = log.h

libicelog_la_SOURCES = log.c

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"

