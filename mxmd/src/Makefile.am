METASOURCES = AUTO
AM_CPPFLAGS =  \
	-DMXMD_EXPORTS \
	@MXMD_CPPFLAGS@ @MXBASE_CPPFLAGS@ @Z_CPPFLAGS@
AM_CXXFLAGS = @Z_CXXFLAGS@
AM_LDFLAGS = @MXMD_LDFLAGS@ @MXMD_SO_LDFLAGS@ \
	@MXBASE_LDFLAGS@ @Z_LDFLAGS@
include_HEADERS = \
	MxMD.hpp MxMDLib.hpp MxMDVersion.hpp MxMDTypes.hpp \
	MxMDCore.hpp MxMDCSV.hpp MxMDStream.hpp \
	MxMDPartition.hpp \
	MxMDBroadcast.hpp \
	MxMDRecord.hpp MxMDReplay.hpp \
	MxMDSubscriber.hpp MxMDPublisher.hpp
lib_LTLIBRARIES = libMxMD.la
libMxMD_la_SOURCES = \
	MxMDPublisher.cpp \
	MxMDSubscriber.cpp \
	MxMD.cpp MxMDLib.cpp MxMDVersion.cpp MxMDCore.cpp \
	MxMDBroadcast.cpp \
	MxMDRecord.cpp MxMDReplay.cpp
libMxMD_la_LIBADD = @MXBASE_LIBS@ @Z_LIBS@ @MXMD_XLIBS@
bin_PROGRAMS = recdump mdrecord
recdump_LDADD = libMxMD.la @MXBASE_LIBS@ @Z_LIBS@ @MXMD_XLIBS@
mdrecord_LDADD = libMxMD.la @MXBASE_LIBS@ @Z_LIBS@ @MXMD_XLIBS@
recdump_SOURCES = recdump.cpp
mdrecord_SOURCES = mdrecord.cpp
