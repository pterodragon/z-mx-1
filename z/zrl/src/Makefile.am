METASOURCES = AUTO
AM_CPPFLAGS = -I$(top_srcdir)/zu/src -I$(top_srcdir)/zm/src \
	-I$(top_srcdir)/zt/src -I$(top_srcdir)/ze/src -I$(top_srcdir)/zi/src \
	-I$(top_srcdir)/zv/src \
	-DZRL_EXPORTS @Z_CPPFLAGS@ @READLINE_CPPFLAGS@
AM_CXXFLAGS = @Z_CXXFLAGS@
AM_LDFLAGS = @Z_LDFLAGS@ @Z_SO_LDFLAGS@ @READLINE_LDFLAGS@
include_HEADERS = ZrlLib.hpp Zrl.hpp
lib_LTLIBRARIES = libZrl.la
bin_PROGRAMS = zcmd
libZrl_la_SOURCES = ZrlLib.cpp Zrl.cpp
libZrl_la_LIBADD  = $(top_builddir)/zv/src/libZv.la \
	$(top_builddir)/zi/src/libZi.la \
	$(top_builddir)/ze/src/libZe.la $(top_builddir)/zt/src/libZt.la \
	$(top_builddir)/zm/src/libZm.la $(top_builddir)/zu/src/libZu.la \
	@READLINE_LIBS@ @Z_IO_LIBS@ @Z_ZT_LIBS@ @Z_MT_LIBS@
zcmd_AM_CPPFLAGS = @Z_CPPFLAGS@ @READLINE_CPPFLAGS@
zcmd_LDADD = libZrl.la $(top_builddir)/zv/src/libZv.la \
	$(top_builddir)/zi/src/libZi.la $(top_builddir)/ze/src/libZe.la \
	$(top_builddir)/zt/src/libZt.la $(top_builddir)/zm/src/libZm.la \
	$(top_builddir)/zu/src/libZu.la \
	@READLINE_LIBS@ @Z_IO_LIBS@ @Z_ZT_LIBS@ @Z_MT_LIBS@
zcmd_SOURCES = zcmd.cpp
