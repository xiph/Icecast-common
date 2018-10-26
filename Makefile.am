## Process this file with automake to produce Makefile.in

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = avl httpp log net thread timing

lib_LTLIBRARIES = libigloo.la

pkginclude_HEADERS = \
    include/igloo/avl.h \
    include/igloo/encoding.h \
    include/igloo/httpp.h \
    include/igloo/igloo.h \
    include/igloo/log.h \
    include/igloo/resolver.h \
    include/igloo/sock.h \
    include/igloo/thread.h \
    include/igloo/timing.h

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
