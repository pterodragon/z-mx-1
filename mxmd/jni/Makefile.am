METASOURCES = AUTO
AM_CPPFLAGS = -I$(top_srcdir)/src \
	@JNI_CPPFLAGS@ @MXMD_CPPFLAGS@ @MXBASE_CPPFLAGS@ @Z_CPPFLAGS@
AM_CXXFLAGS = @Z_CXXFLAGS@
AM_LDFLAGS = @MXMD_LDFLAGS@ @MXMD_SO_LDFLAGS@ @MXBASE_LDFLAGS@ @Z_LDFLAGS@
include_HEADERS = \
	MxMDDerivativesJNI.hpp MxMDFeedJNI.hpp MxMDLibJNI.hpp \
	MxMDOBSideJNI.hpp MxMDOrderBookJNI.hpp MxMDOrderJNI.hpp \
	MxMDPxLevelJNI.hpp MxMDSecurityJNI.hpp MxMDTickSizeTblJNI.hpp \
	MxMDTradeJNI.hpp MxMDVenueJNI.hpp
lib_LTLIBRARIES = libMxMDJNI.la
libMxMDJNI_la_SOURCES = \
	MxMDDerivativesJNI.cpp MxMDFeedJNI.cpp MxMDJNI.cpp MxMDLibJNI.cpp \
	MxMDOBSideJNI.cpp MxMDOrderBookJNI.cpp MxMDOrderJNI.cpp \
	MxMDPxLevelJNI.cpp MxMDSecurityJNI.cpp MxMDTickSizeTblJNI.cpp \
	MxMDTradeJNI.cpp MxMDVenueJNI.cpp
libMxMDJNI_la_LIBADD = $(top_builddir)/src/libMxMD.la \
	@MXBASE_LIBS@ -lZJNI @Z_LIBS@ @MXMD_XLIBS@
