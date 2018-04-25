METASOURCES = AUTO
AM_CPPFLAGS = -I$(top_srcdir)/zu/src -I$(top_srcdir)/zm/src \
	-I$(top_srcdir)/zt/src -I$(top_srcdir)/ze/src -I$(top_srcdir)/zi/src \
	-DZV_EXPORTS @Z_CPPFLAGS@
AM_CXXFLAGS = @Z_CXXFLAGS@
AM_LDFLAGS = @Z_LDFLAGS@
include_HEADERS = ZvCf.hpp ZvCSV.hpp ZvError.hpp ZvEnum.hpp ZvHashMgrCf.hpp \
	ZvHeapCf.hpp ZvMultiplexCf.hpp ZvSchedulerCf.hpp ZvCxnOptionsCf.hpp \
	ZvDaemon.hpp ZvRegexError.hpp ZvLib.hpp ZvCmd.hpp ZvDateError.hpp \
	ZvRingCf.hpp ZvThreadCf.hpp
lib_LTLIBRARIES = libZv.la
bin_PROGRAMS = zproxy
libZv_la_LDFLAGS = @Z_SO_LDFLAGS@
libZv_la_SOURCES = ZvCf.cpp ZvCSV.cpp ZvDaemon.cpp ZvLib.cpp \
	ZvCmd.cpp
libZv_la_LIBADD = $(top_builddir)/zi/src/libZi.la \
	$(top_builddir)/ze/src/libZe.la \
	$(top_builddir)/zt/src/libZt.la $(top_builddir)/zm/src/libZm.la \
	$(top_builddir)/zu/src/libZu.la \
	@Z_IO_LIBS@ @Z_ZT_LIBS@ @Z_MT_LIBS@
zproxy_AM_CPPFLAGS = @Z_CPPFLAGS@
zproxy_LDADD = libZv.la $(top_builddir)/zi/src/libZi.la \
	$(top_builddir)/ze/src/libZe.la $(top_builddir)/zt/src/libZt.la \
	$(top_builddir)/zm/src/libZm.la $(top_builddir)/zu/src/libZu.la \
	@Z_IO_LIBS@ @Z_ZT_LIBS@ @Z_MT_LIBS@
zproxy_SOURCES = zproxy.cpp
