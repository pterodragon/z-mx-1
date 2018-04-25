METASOURCES = AUTO
AM_CPPFLAGS = -I$(top_srcdir)/zu/src \
	-DZLZ4_EXPORTS @Z_CPPFLAGS@
AM_CXXFLAGS = @Z_CXXFLAGS@
AM_LDFLAGS = @Z_LDFLAGS@ @Z_SO_LDFLAGS@
include_HEADERS = Zlz4Lib.hpp lz4.h
lib_LTLIBRARIES = libZlz4.la
libZlz4_la_SOURCES = Zlz4Lib.cpp lz4.c
# libZlz4_la_LIBADD =
