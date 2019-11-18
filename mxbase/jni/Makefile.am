METASOURCES = AUTO
AM_CPPFLAGS = -I$(top_srcdir)/src -DMXBASE_EXPORTS \
	@JNI_CPPFLAGS@ @MXBASE_CPPFLAGS@ @Z_CPPFLAGS@
AM_CXXFLAGS = @Z_CXXFLAGS@
AM_LDFLAGS = @MXBASE_LDFLAGS@ @MXBASE_SO_LDFLAGS@ @Z_LDFLAGS@
pkginclude_HEADERS = MxBaseJNI.hpp \
	MxSideJNI.hpp MxInstrIDSrcJNI.hpp MxPutCallJNI.hpp MxTickDirJNI.hpp \
	MxTradingSessionJNI.hpp MxTradingStatusJNI.hpp \
	MxInstrKeyJNI.hpp MxFutKeyJNI.hpp MxOptKeyJNI.hpp MxUniKeyJNI.hpp \
	MxDecimalJNI.hpp
lib_LTLIBRARIES = libMxBaseJNI.la libMxJNITest.la
libMxBaseJNI_la_SOURCES = MxBaseJNI.cpp \
	MxSideJNI.cpp MxInstrIDSrcJNI.cpp MxPutCallJNI.cpp MxTickDirJNI.cpp \
	MxTradingSessionJNI.cpp MxTradingStatusJNI.cpp \
	MxInstrKeyJNI.cpp MxFutKeyJNI.cpp MxOptKeyJNI.cpp MxUniKeyJNI.cpp \
	MxDecimalJNI.cpp
libMxBaseJNI_la_LIBADD = $(top_builddir)/src/libMxBase.la \
	-lZJNI @Z_LIBS@ @MXBASE_XLIBS@
libMxJNITest_la_SOURCES = MxJNITest.cpp
libMxJNITest_la_LIBADD = $(top_builddir)/src/libMxBase.la \
	-lZJNI @Z_LIBS@ @MXBASE_XLIBS@
