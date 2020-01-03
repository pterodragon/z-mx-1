METASOURCES = AUTO
AM_CPPFLAGS = -DMXBASE_EXPORTS @MXBASE_CPPFLAGS@ @Z_CPPFLAGS@
AM_CXXFLAGS = @Z_CXXFLAGS@
AM_LDFLAGS = @MXBASE_LDFLAGS@ @MXBASE_SO_LDFLAGS@ @Z_LDFLAGS@
pkginclude_HEADERS = \
	MxBase.hpp MxBaseLib.hpp MxBaseVersion.hpp \
	MxCSV.hpp MxMultiplex.hpp MxScheduler.hpp \
	MxValWindow.hpp \
	MxTxDB.hpp MxRxDB.hpp
lib_LTLIBRARIES = libMxBase.la
libMxBase_la_SOURCES = MxBaseLib.cpp MxBaseVersion.cpp MxEngine.cpp \
	MxTelemetry.cpp
libMxBase_la_LIBADD = @Z_LIBS@ @MXBASE_XLIBS@
bin_PROGRAMS = mcap mcmerge mcreplay telcap
mcap_LDADD = libMxBase.la @Z_LIBS@ @MXBASE_XLIBS@
mcmerge_LDADD = libMxBase.la @Z_LIBS@ @MXBASE_XLIBS@
mcreplay_LDADD = libMxBase.la @Z_LIBS@ @MXBASE_XLIBS@
telcap_LDADD = libMxBase.la -lZdb @Z_LIBS@ @MXBASE_XLIBS@
mcap_SOURCES = mcap.cpp
mcmerge_SOURCES = mcmerge.cpp
mcreplay_SOURCES = mcreplay.cpp
telcap_SOURCES = telcap.cpp
