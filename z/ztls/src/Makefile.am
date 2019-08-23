METASOURCES = AUTO
AM_CPPFLAGS = -I$(top_srcdir)/zu/src -I$(top_srcdir)/zm/src \
	-I$(top_srcdir)/zt/src -I$(top_srcdir)/ze/src -I${top_srcdir}/zi/src \
	-DZTLS_EXPORTS @Z_CPPFLAGS@
AM_CXXFLAGS = @Z_CXXFLAGS@
AM_LDFLAGS = @Z_LDFLAGS@ @Z_SO_LDFLAGS@
include_HEADERS = ZTLSLib.hpp ZTLS.hpp
lib_LTLIBRARIES = libZTLS.la
libZTLS_la_SOURCES = ZTLSLib.cpp ZTLS.cpp
libZTLS_la_LIBADD = $(top_builddir)/ze/src/libZi.la \
	$(top_builddir)/ze/src/libZe.la \
	$(top_builddir)/zt/src/libZt.la \
	$(top_builddir)/zm/src/libZm.la \
	$(top_builddir)/zu/src/libZu.la \
	@Z_TLS_LIBS@ @Z_IO_LIBS@ @Z_ZT_LIBS@ @Z_MT_LIBS@
