METASOURCES = AUTO
AM_CPPFLAGS = -I$(top_srcdir)/src -DMXMD_EXPORTS \
	@JNI_CPPFLAGS@ @MXMD_CPPFLAGS@ @MXBASE_CPPFLAGS@ @Z_CPPFLAGS@
AM_CXXFLAGS = @Z_CXXFLAGS@
AM_LDFLAGS = @MXMD_LDFLAGS@ @MXMD_SO_LDFLAGS@ @MXBASE_LDFLAGS@ @Z_LDFLAGS@
include_HEADERS = MxMDJNI.hpp \
	MxMDSeverityJNI.hpp MxMDOrderIDScopeJNI.hpp MxMDVenueFlagsJNI.hpp \
	MxMDLibJNI.hpp MxMDFeedJNI.hpp MxMDVenueJNI.hpp MxMDTickSizeTblJNI.hpp \
	MxMDInstrumentJNI.hpp MxMDDerivativesJNI.hpp \
	MxMDOrderBookJNI.hpp MxMDOBSideJNI.hpp MxMDPxLevelJNI.hpp \
	MxMDOrderJNI.hpp MxMDTradeJNI.hpp \
	MxMDLibHandlerJNI.hpp MxMDInstrHandlerJNI.hpp \
	MxMDInstrHandleJNI.hpp MxMDOBHandleJNI.hpp \
	MxMDExceptionJNI.hpp \
	MxMDTickSizeJNI.hpp MxMDSegmentJNI.hpp \
	MxMDInstrRefDataJNI.hpp MxMDLotSizesJNI.hpp \
	MxMDL1DataJNI.hpp MxMDOBSideDataJNI.hpp MxMDPxLvlDataJNI.hpp \
	MxMDOrderDataJNI.hpp MxMDTradeDataJNI.hpp
lib_LTLIBRARIES = libMxMDJNI.la libMxMDJNITest.la
libMxMDJNI_la_SOURCES = MxMDJNI.cpp \
	MxMDSeverityJNI.cpp MxMDOrderIDScopeJNI.cpp MxMDVenueFlagsJNI.cpp \
	MxMDLibJNI.cpp MxMDFeedJNI.cpp MxMDVenueJNI.cpp MxMDTickSizeTblJNI.cpp \
	MxMDInstrumentJNI.cpp MxMDDerivativesJNI.cpp \
	MxMDOrderBookJNI.cpp MxMDOBSideJNI.cpp MxMDPxLevelJNI.cpp \
	MxMDOrderJNI.cpp MxMDTradeJNI.cpp \
	MxMDLibHandlerJNI.cpp MxMDInstrHandlerJNI.cpp \
	MxMDInstrHandleJNI.cpp MxMDOBHandleJNI.cpp \
	MxMDExceptionJNI.cpp \
	MxMDTickSizeJNI.cpp MxMDSegmentJNI.cpp \
	MxMDInstrRefDataJNI.cpp MxMDLotSizesJNI.cpp \
	MxMDL1DataJNI.cpp MxMDOBSideDataJNI.cpp MxMDPxLvlDataJNI.cpp \
	MxMDOrderDataJNI.cpp MxMDTradeDataJNI.cpp
libMxMDJNI_la_LIBADD = $(top_builddir)/src/libMxMD.la \
	-lMxBaseJNI @MXBASE_LIBS@ -lZJNI @Z_LIBS@ @MXMD_XLIBS@
libMxMDJNITest_la_SOURCES = MxMDJNITest.hpp MxMDJNITest.cpp
libMxMDJNITest_la_LIBADD = $(top_builddir)/src/libMxMD.la \
	@MXBASE_LIBS@ -lZJNI @Z_LIBS@ @MXMD_XLIBS@
