## Process this file with automake to produce Makefile.in

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = avl httpp log net thread timing

lib_LTLIBRARIES = libpermafrost.la

libpermafrost_la_SOURCES = libpermafrost.c
libpermafrost_la_LIBADD = \
    avl/libiceavl.la \
    httpp/libicehttpp.la \
    log/libicelog.la \
    net/libicenet.la \
    thread/libicethread.la \
    timing/libicetiming.la

pkgconfig_DATA = permafrost.pc

static:
	$(MAKE) all LDFLAGS="${LDFLAGS} -all-static"
