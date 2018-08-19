METASOURCES = AUTO
AM_CPPFLAGS = -I$(top_srcdir)/src \
	@JNI_CPPFLAGS@ @MXBASE_CPPFLAGS@ @Z_CPPFLAGS@
AM_CXXFLAGS = @Z_CXXFLAGS@
AM_LDFLAGS = @MXBASE_LDFLAGS@ @MXBASE_SO_LDFLAGS@ @Z_LDFLAGS@
lib_LTLIBRARIES = libMxJNITest.la libMxBaseJNI.la
libMxJNITest_la_SOURCES = MxJNITest.cpp MxBaseJNI.cpp
libMxJNITest_la_LIBADD = $(top_builddir)/src/libMxBase.la \
	@Z_LIBS@ @MXBASE_XLIBS@
libMxBaseJNI_la_SOURCES = MxBaseJNI.cpp
libMxBaseJNI_la_LIBADD = $(top_builddir)/src/libMxBase.la \
	@Z_LIBS@ @MXBASE_XLIBS@
