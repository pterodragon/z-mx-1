METASOURCES = AUTO
AM_CPPFLAGS = -I$(top_srcdir)/zu/src -I$(top_srcdir)/zm/src
	-DZFB_EXPORTS @Z_CPPFLAGS@
AM_CXXFLAGS = @Z_CXXFLAGS@
AM_LDFLAGS = @Z_LDFLAGS@ @Z_SO_LDFLAGS@
pkginclude_HEADERS = ZfbLib.hpp Zfb.hpp
lib_LTLIBRARIES = libZfb.la
libZfb_la_SOURCES = ZfbLib.cpp
libZfb_la_LIBADD = \
	$(top_builddir)/zm/src/libZm.la \
	$(top_builddir)/zu/src/libZu.la \
	@FLATBUFFERS_LIBS@ @Z_MT_LIBS@
