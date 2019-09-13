METASOURCES = AUTO
AM_CPPFLAGS = -I$(top_srcdir)/zu/src \
	-I$(top_srcdir)/zm/src -I$(top_srcdir)/zt/src \
	-I$(top_srcdir)/ze/src -I$(top_srcdir)/zi/src \
	-I$(top_srcdir)/zfb/src \
	@Z_CPPFLAGS@
AM_CXXFLAGS = @Z_CXXFLAGS@
AM_LDFLAGS = @Z_LDFLAGS@ @Z_SO_LDFLAGS@
LDADD = $(top_builddir)/zfb/src/libZfb.la \
	$(top_builddir)/zi/src/libZi.la $(top_builddir)/ze/src/libZe.la \
	$(top_builddir)/zt/src/libZt.la $(top_builddir)/zm/src/libZm.la \
	$(top_builddir)/zu/src/libZu.la \
	@FLATBUFFERS_LIBS@ @Z_IO_LIBS@ @Z_ZT_LIBS@ @Z_MT_LIBS@
bin_PROGRAMS = zfbtest
%_generated.h: %.fbs
	flatc --cpp $<
BUILT_SOURCES = user_generated.h zfbtest_generated.h
zfbtest_SOURCES = zfbtest.cpp user_generated.h zfbtest_generated.h
