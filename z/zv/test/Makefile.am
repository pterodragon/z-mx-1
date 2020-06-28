METASOURCES = AUTO
AM_CPPFLAGS = -I$(top_srcdir)/zu/src -I$(top_srcdir)/zm/src \
	-I$(top_srcdir)/zt/src -I$(top_srcdir)/ze/src -I$(top_srcdir)/zi/src \
	-I$(top_srcdir)/ztls/src \
	-I$(top_srcdir)/zfb/src \
	-I$(top_srcdir)/zv/src \
	@Z_CPPFLAGS@
AM_CXXFLAGS = @Z_CXXFLAGS@
AM_LDFLAGS = @Z_LDFLAGS@
LDADD = $(top_builddir)/zv/src/libZv.la \
	$(top_builddir)/zfb/src/libZfb.la \
	$(top_builddir)/ztls/src/libZtls.la \
	$(top_builddir)/zi/src/libZi.la \
	$(top_builddir)/ze/src/libZe.la $(top_builddir)/zt/src/libZt.la \
	$(top_builddir)/zm/src/libZm.la $(top_builddir)/zu/src/libZu.la \
	@MBEDTLS_LIBS@ @Z_IO_LIBS@ @Z_ZT_LIBS@ @Z_MT_LIBS@
noinst_PROGRAMS = \
	CfTest CSVTest CSVWriteTest CmdTest CfFlatten UserDBTest SeriesBufTest
CfTest_SOURCES = CfTest.cpp
CSVTest_SOURCES = CSVTest.cpp
CmdTest_SOURCES = CmdTest.cpp
CSVWriteTest_SOURCES = CSVWriteTest.cpp
CfFlatten_SOURCES = CfFlatten.cpp
UserDBTest_SOURCES = UserDBTest.cpp
SeriesBufTest_SOURCES = SeriesBufTest.cpp
