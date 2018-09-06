METASOURCES = AUTO
AM_CPPFLAGS = -I$(top_srcdir)/src \
	@MXMD_CPPFLAGS@ @MXBASE_CPPFLAGS@ @Z_CPPFLAGS@
AM_CXXFLAGS = @Z_CXXFLAGS@
AM_LDFLAGS = @MXMD_LDFLAGS@ @MXBASE_LDFLAGS@ @Z_LDFLAGS@
LDADD = $(top_builddir)/src/libMxMD.la @MXBASE_LIBS@ @Z_LIBS@ @MXMD_XLIBS@
bin_PROGRAMS = \
	mdsample_standalone mdsample_symlist mdsample_interactive mdsample_publisher
mdsample_standalone_SOURCES = mdsample_standalone.cpp
mdsample_symlist_SOURCES = mdsample_symlist.cpp
mdsample_interactive_SOURCES = mdsample_interactive.cpp
mdsample_publisher_SOURCES = mdsample_publisher.cpp
