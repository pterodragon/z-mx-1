METASOURCES = AUTO
AM_CPPFLAGS =  \
	-DMXMD_EXPORTS \
	@MXMD_CPPFLAGS@ @MXBASE_CPPFLAGS@ @Z_CPPFLAGS@
AM_CXXFLAGS = @Z_CXXFLAGS@
AM_LDFLAGS = @MXMD_LDFLAGS@ @MXMD_SO_LDFLAGS@ \
	@MXBASE_LDFLAGS@ @Z_LDFLAGS@
include_HEADERS = \
	MxMD.hpp MxMDLib.hpp MxMDVersion.hpp MxMDTypes.hpp \
	MxMDCore.hpp MxMDCmd.hpp MxMDCSV.hpp MxMDFrame.hpp MxMDStream.hpp
lib_LTLIBRARIES = libMxMD.la
libMxMD_la_SOURCES = \
	MxMD.cpp MxMDLib.cpp MxMDVersion.cpp MxMDCore.cpp
libMxMD_la_LIBADD = @MXBASE_LIBS@ @Z_LIBS@ @MXMD_XLIBS@
bin_PROGRAMS = recdump mdrecord mcap mcmerge mcreplay
recdump_LDADD = libMxMD.la @MXBASE_LIBS@ @Z_LIBS@ @MXMD_XLIBS@
mdrecord_LDADD = libMxMD.la @MXBASE_LIBS@ @Z_LIBS@ @MXMD_XLIBS@
mcap_LDADD = libMxMD.la @MXBASE_LIBS@ @Z_LIBS@ @MXMD_XLIBS@
mcmerge_LDADD = libMxMD.la @MXBASE_LIBS@ @Z_LIBS@ @MXMD_XLIBS@
mcreplay_LDADD = libMxMD.la @MXBASE_LIBS@ @Z_LIBS@ @MXMD_XLIBS@
recdump_SOURCES = recdump.cpp
mdrecord_SOURCES = mdrecord.cpp
mcap_SOURCES = mcap.cpp
mcmerge_SOURCES = mcmerge.cpp
mcreplay_SOURCES = mcreplay.cpp
