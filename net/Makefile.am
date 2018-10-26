## Process this with automake to create Makefile.in

AUTOMAKE_OPTIONS = foreign

EXTRA_DIST = BUILDING COPYING README TODO test_resolver.c

noinst_LTLIBRARIES = libicenet.la

libicenet_la_SOURCES = sock.c resolver.c

AM_CPPFLAGS = -I$(srcdir)/..

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@

