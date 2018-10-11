## Process this with automake to create Makefile.in

AUTOMAKE_OPTIONS = foreign

EXTRA_DIST = BUILDING COPYING README TODO avl.dsp test.c

noinst_LTLIBRARIES = libiceavl.la
pkginclude_HEADERS = avl.h

libiceavl_la_SOURCES = avl.c

AM_CPPFLAGS = -I$(srcdir)/..

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"

