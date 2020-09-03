METASOURCES = AUTO
AM_CPPFLAGS = -I$(top_srcdir)/zu/src -I$(top_srcdir)/zm/src \
	-I$(top_srcdir)/zt/src -I$(top_srcdir)/ze/src -I$(top_srcdir)/zi/src \
	-I$(top_srcdir)/ztls/src -I$(top_srcdir)/zfb/src \
	-I$(top_srcdir)/zv/src \
	-DZGTK_EXPORTS @Z_CPPFLAGS@
AM_CFLAGS = @GTK_CFLAGS@
AM_CXXFLAGS = @Z_CXXFLAGS@ @GTK_CFLAGS@
AM_LDFLAGS = @Z_LDFLAGS@ @GTK_LDFLAGS@
pkginclude_HEADERS = ZGtkLib.hpp \
	ZGtkApp.hpp ZGtkCallback.hpp ZGtkTreeModel.hpp
lib_LTLIBRARIES = libZGtk.la
libZGtk_la_LDFLAGS = @Z_SO_LDFLAGS@
libZGtk_la_SOURCES = ZGtkLib.cpp ZGtkApp.cpp
libZGtk_la_LIBADD = $(top_builddir)/zv/src/libZv.la \
	$(top_builddir)/zfb/src/libZfb.la $(top_builddir)/ztls/src/libZtls.la \
	$(top_builddir)/zi/src/libZi.la $(top_builddir)/ze/src/libZe.la \
	$(top_builddir)/zt/src/libZt.la $(top_builddir)/zm/src/libZm.la \
	$(top_builddir)/zu/src/libZu.la \
	@GTK_LIBS@ @FLATBUFFERS_LIBS@ @MBEDTLS_LIBS@ \
	@Z_IO_LIBS@ @Z_ZT_LIBS@ @Z_MT_LIBS@
bin_PROGRAMS = zdash
zdash_CPPFLAGS = $(AM_CPPFLAGS) -I$(top_srcdir)/zrl/src -I$(top_srcdir)/zdf/src
zdash_LDADD = libZGtk.la $(top_builddir)/zdf/src/libZdf.la $(top_builddir)/zrl/src/libZrl.la $(libZGtk_la_LIBADD)
zdash_SOURCES = zdash.cpp
