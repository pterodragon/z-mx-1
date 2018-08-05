METASOURCES = AUTO
AM_CPPFLAGS = -I$(top_srcdir)/src \
	@MXBASE_CPPFLAGS@ @Z_CPPFLAGS@
AM_CXXFLAGS = @Z_CXXFLAGS@
AM_LDFLAGS = @MXBASE_LDFLAGS@ @Z_LDFLAGS@
LDADD = $(top_builddir)/src/libMxBase.la @Z_LIBS@ @MXBASE_XLIBS@
bin_PROGRAMS = MxEngineTest MxFixedTest
MxEngineTest_SOURCES = MxEngineTest.cpp
MxFixedTest_SOURCES = MxFixedTest.cpp
