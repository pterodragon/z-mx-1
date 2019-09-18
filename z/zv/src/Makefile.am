METASOURCES = AUTO
AM_CPPFLAGS = -I$(top_srcdir)/zu/src -I$(top_srcdir)/zm/src \
	-I$(top_srcdir)/zt/src -I$(top_srcdir)/ze/src -I$(top_srcdir)/zi/src \
	-I$(top_srcdir)/ztls/src -I$(top_srcdir)/zfb/src \
	-DZV_EXPORTS @Z_CPPFLAGS@
AM_CXXFLAGS = @Z_CXXFLAGS@
AM_LDFLAGS = @Z_LDFLAGS@
FBS = \
	key.fbs \
	perm.fbs \
	role.fbs \
	user.fbs \
	userdb.fbs \
	loginack.fbs \
	loginreq.fbs \
	userdbreq.fbs \
	userdback.fbs \
	zcmd.fbs \
	zcmdreq.fbs \
	zcmdack.fbs
FBS_H = ${FBS:%.fbs=%_generated.h}
${FBS_H}: %_generated.h: fbs/%.fbs
	flatc -c -I fbs $<
BUILT_SOURCES = ${FBS_H}
CLEANFILES = ${FBS_H}
include_HEADERS = ZvCf.hpp ZvCSV.hpp ZvError.hpp ZvEnum.hpp ZvHashCSV.hpp \
	ZvHeapCSV.hpp ZvMultiplexCf.hpp ZvSchedulerCf.hpp ZvCxnOptionsCf.hpp \
	ZvDaemon.hpp ZvRegexError.hpp ZvLib.hpp ZvCmd.hpp ZvDateError.hpp \
	ZvRingCf.hpp ZvThreadCf.hpp ZvUserDB.hpp ${FBS_H}
lib_LTLIBRARIES = libZv.la
bin_PROGRAMS = zproxy
libZv_la_LDFLAGS = @Z_SO_LDFLAGS@
libZv_la_SOURCES = ZvCf.cpp ZvCSV.cpp ZvDaemon.cpp ZvLib.cpp \
	ZvCmd.cpp ZvUserDB.cpp
libZv_la_LIBADD = $(top_builddir)/zfb/src/libZfb.la \
	$(top_builddir)/ztls/src/libZtls.la \
	$(top_builddir)/zi/src/libZi.la $(top_builddir)/ze/src/libZe.la \
	$(top_builddir)/zt/src/libZt.la $(top_builddir)/zm/src/libZm.la \
	$(top_builddir)/zu/src/libZu.la \
	@Z_IO_LIBS@ @Z_ZT_LIBS@ @Z_MT_LIBS@
zproxy_AM_CPPFLAGS = @Z_CPPFLAGS@
zproxy_LDADD = libZv.la $(top_builddir)/zi/src/libZi.la \
	$(top_builddir)/ze/src/libZe.la $(top_builddir)/zt/src/libZt.la \
	$(top_builddir)/zm/src/libZm.la $(top_builddir)/zu/src/libZu.la \
	@Z_IO_LIBS@ @Z_ZT_LIBS@ @Z_MT_LIBS@
zproxy_SOURCES = zproxy.cpp
