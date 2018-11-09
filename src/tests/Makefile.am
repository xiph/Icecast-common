## Process this file with automake to produce Makefile.in

#######################
# libigloo unit tests #
#######################

TEST_LOG_DRIVER = env AM_TAP_AWK='$(AWK)' $(SHELL) \
    $(top_srcdir)/build-aux/tap-driver.sh

check_PROGRAMS =

#
# Helper library for TAP tests
#

libice_ctest_la_SOURCES = %reldir%/ctest_lib.c %reldir%/ctest_lib.h
noinst_LTLIBRARIES = libice_ctest.la

#
# Test programs
#

ctest_suite_test_SOURCES = %reldir%/ctest_suite.c
ctest_suite_test_LDADD = libice_ctest.la
check_PROGRAMS += ctest_suite.test

ctest_refobject_test_SOURCES = %reldir%/ctest_refobject.c
ctest_refobject_test_LDADD = libice_ctest.la \
			     thread/libicethread.la \
			     avl/libiceavl.la \
			     src/ro.o
check_PROGRAMS += ctest_refobject.test

ctest_buffer_test_SOURCES = %reldir%/ctest_buffer.c
ctest_buffer_test_LDADD = libice_ctest.la \
			  thread/libicethread.la \
			  avl/libiceavl.la \
			  src/ro.o \
			  src/buffer.o
check_PROGRAMS += ctest_buffer.test

ctest_list_test_SOURCES = %reldir%/ctest_list.c
ctest_list_test_LDADD = libice_ctest.la \
			thread/libicethread.la \
			avl/libiceavl.la \
			src/ro.o \
			src/list.o
check_PROGRAMS += ctest_list.test

ctest_reportxml_test_SOURCES = %reldir%/ctest_reportxml.c
ctest_reportxml_test_LDADD = libice_ctest.la \
			thread/libicethread.la \
			avl/libiceavl.la \
			src/ro.o \
			src/reportxml.o
check_PROGRAMS += ctest_reportxml.test

# Add all programs to TESTS
TESTS = $(check_PROGRAMS)
