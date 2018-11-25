METASOURCES = AUTO
AM_CPPFLAGS = -I$(top_srcdir)/src \
	@JNI_CPPFLAGS@ @MXBASE_CPPFLAGS@ @Z_CPPFLAGS@
AM_CXXFLAGS = @Z_CXXFLAGS@
AM_LDFLAGS = @MXBASE_LDFLAGS@ @MXBASE_SO_LDFLAGS@ @Z_LDFLAGS@
include_HEADERS = MxBaseJNI.hpp \
	MxInstrKeyJNI.hpp MxFutKeyJNI.hpp MxOptKeyJNI.hpp
lib_LTLIBRARIES = libMxBaseJNI.la libMxJNITest.la
libMxBaseJNI_la_SOURCES = MxBaseJNI.cpp \
	MxInstrKeyJNI.cpp MxFutKeyJNI.cpp MxOptKeyJNI.cpp
libMxBaseJNI_la_LIBADD = $(top_builddir)/src/libMxBase.la \
	-lZJNI @Z_LIBS@ @MXBASE_XLIBS@
libMxJNITest_la_SOURCES = MxJNITest.cpp
libMxJNITest_la_LIBADD = $(top_builddir)/src/libMxBase.la \
	-lZJNI @Z_LIBS@ @MXBASE_XLIBS@
