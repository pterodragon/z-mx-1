METASOURCES = AUTO
AM_CPPFLAGS = -I$(top_srcdir)/src \
	@JNI_CPPFLAGS@ @MXMD_CPPFLAGS@ @MXBASE_CPPFLAGS@ @Z_CPPFLAGS@
AM_CXXFLAGS = @Z_CXXFLAGS@
AM_LDFLAGS = @MXMD_LDFLAGS@ @MXMD_SO_LDFLAGS@ @MXBASE_LDFLAGS@ @Z_LDFLAGS@
include_HEADERS = MxMDJNI.hpp \
	MxMDLibJNI.hpp MxMDLibHandlerJNI.hpp \
	MxMDFeedJNI.hpp MxMDVenueJNI.hpp MxMDTickSizeTblJNI.hpp \
	MxMDSecurityJNI.hpp MxMDDerivativesJNI.hpp \
	MxMDOrderBookJNI.hpp MxMDOBSideJNI.hpp MxMDPxLevelJNI.hpp \
	MxMDOrderJNI.hpp MxMDTradeJNI.hpp \
	MxMDTickSizeJNI.hpp MxMDSegmentJNI.hpp \
	MxMDL1DataJNI.hpp MxMDSecRefDataJNI.hpp
lib_LTLIBRARIES = libMxMDJNI.la libMxMDJNITest.la
libMxMDJNI_la_SOURCES = MxMDJNI.cpp \
	MxMDLibJNI.cpp MxMDLibHandlerJNI.cpp \
	MxMDFeedJNI.cpp MxMDVenueJNI.cpp MxMDTickSizeTblJNI.cpp \
	MxMDSecurityJNI.cpp MxMDDerivativesJNI.cpp \
	MxMDOrderBookJNI.cpp MxMDOBSideJNI.cpp MxMDPxLevelJNI.cpp \
	MxMDOrderJNI.cpp MxMDTradeJNI.cpp \
	MxMDTickSizeJNI.cpp MxMDSegmentJNI.cpp \
	MxMDL1DataJNI.cpp MxMDSecRefDataJNI.cpp
libMxMDJNI_la_LIBADD = $(top_builddir)/src/libMxMD.la \
	-lMxBaseJNI @MXBASE_LIBS@ -lZJNI @Z_LIBS@ @MXMD_XLIBS@
libMxMDJNITest_la_SOURCES = MxMDJNITest.hpp MxMDJNITest.cpp
libMxMDJNITest_la_LIBADD = $(top_builddir)/src/libMxMD.la \
	@MXBASE_LIBS@ -lZJNI @Z_LIBS@ @MXMD_XLIBS@
