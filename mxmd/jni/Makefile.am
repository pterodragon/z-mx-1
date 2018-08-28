METASOURCES = AUTO
AM_CPPFLAGS = -I$(top_srcdir)/src \
	@JNI_CPPFLAGS@ @MXMD_CPPFLAGS@ @MXBASE_CPPFLAGS@ @Z_CPPFLAGS@
AM_CXXFLAGS = @Z_CXXFLAGS@
AM_LDFLAGS = @MXMD_LDFLAGS@ @MXMD_SO_LDFLAGS@ @MXBASE_LDFLAGS@ @Z_LDFLAGS@
include_HEADERS = MxMDJNI.hpp \
	MxMDDerivativesJNI.hpp MxMDFeedJNI.hpp MxMDLibJNI.hpp \
	MxMDOBSideJNI.hpp MxMDOrderBookJNI.hpp MxMDOrderJNI.hpp \
	MxMDPxLevelJNI.hpp MxMDSecurityJNI.hpp MxMDTickSizeTblJNI.hpp \
	MxMDTradeJNI.hpp MxMDVenueJNI.hpp \
	MxMDTickSizeJNI.hpp
lib_LTLIBRARIES = libMxMDJNI.la libMxMDJNITest.la
libMxMDJNI_la_SOURCES = MxMDJNI.cpp \
	MxMDDerivativesJNI.cpp MxMDFeedJNI.cpp MxMDLibJNI.cpp \
	MxMDOBSideJNI.cpp MxMDOrderBookJNI.cpp MxMDOrderJNI.cpp \
	MxMDPxLevelJNI.cpp MxMDSecurityJNI.cpp MxMDTickSizeTblJNI.cpp \
	MxMDTradeJNI.cpp MxMDVenueJNI.cpp \
	MxMDTickSizeJNI.cpp
libMxMDJNI_la_LIBADD = $(top_builddir)/src/libMxMD.la \
	-lMxBaseJNI @MXBASE_LIBS@ -lZJNI @Z_LIBS@ @MXMD_XLIBS@
libMxMDJNITest_la_SOURCES = MxMDJNITest.hpp MxMDJNITest.cpp
libMxMDJNITest_la_LIBADD = $(top_builddir)/src/libMxMD.la \
	@MXBASE_LIBS@ -lZJNI @Z_LIBS@ @MXMD_XLIBS@
