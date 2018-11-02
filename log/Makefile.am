## Process this with automake to create Makefile.in

AUTOMAKE_OPTIONS = foreign

EXTRA_DIST = test.c

noinst_LTLIBRARIES = libicelog.la

libicelog_la_SOURCES = log.c

AM_CFLAGS = -I$(top_srcdir)/include

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"

