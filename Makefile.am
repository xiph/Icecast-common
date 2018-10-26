## Process this file with automake to produce Makefile.in

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = avl httpp log net thread timing

lib_LTLIBRARIES = libigloo.la

pkginclude_HEADERS = include/igloo.h

libigloo_la_SOURCES = src/libigloo.c
libigloo_la_LIBADD = \
    avl/libiceavl.la \
    httpp/libicehttpp.la \
    log/libicelog.la \
    net/libicenet.la \
    thread/libicethread.la \
    timing/libicetiming.la

pkgconfig_DATA = igloo.pc

static:
	$(MAKE) all LDFLAGS="${LDFLAGS} -all-static"
