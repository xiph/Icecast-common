## Process this with automake to create Makefile.in

AUTOMAKE_OPTIONS = foreign

EXTRA_DIST = BUILDING COPYING README TODO

noinst_LTLIBRARIES = libicetiming.la

libicetiming_la_SOURCES = timing.c

AM_CFLAGS = -I$(top_srcdir)/include

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"

