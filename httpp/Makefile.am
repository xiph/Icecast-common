## Process this with automake to create Makefile.in

AUTOMAKE_OPTIONS = foreign

noinst_LTLIBRARIES = libicehttpp.la

libicehttpp_la_SOURCES = httpp.c encoding.c
AM_CPPFLAGS = -I$(srcdir)/..

# SCCS stuff (for BitKeeper)
GET = true

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"

