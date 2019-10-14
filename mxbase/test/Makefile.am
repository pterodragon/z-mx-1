METASOURCES = AUTO
AM_CPPFLAGS = -I$(top_srcdir)/src \
	@MXBASE_CPPFLAGS@ @Z_CPPFLAGS@
AM_CXXFLAGS = @Z_CXXFLAGS@
AM_LDFLAGS = @MXBASE_LDFLAGS@ @Z_LDFLAGS@
LDADD = $(top_builddir)/src/libMxBase.la @Z_LIBS@ @MXBASE_XLIBS@
noinst_PROGRAMS = MxEngineTest MxValueTest MxTelServer MxVWTest
MxEngineTest_SOURCES = MxEngineTest.cpp
MxValueTest_SOURCES = MxValueTest.cpp
MxTelServer_SOURCES = MxTelServer.cpp
MxVWTest_SOURCES = MxVWTest.cpp
