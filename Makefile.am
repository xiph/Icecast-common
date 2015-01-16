AUTOMAKE_OPTIONS = foreign

SUBDIRS = avl net thread httpp log timing

lib_LTLIBRARIES = libicecast.la
libshout_la_SOURCES = libicecast.c
libicecast_la_LIBADD = net/libicenet.la timing/libicetiming.la avl/libiceavl.la httpp/libicehttpp.la thread/libicethread.la
