METASOURCES = AUTO
AM_CPPFLAGS = -I$(top_srcdir)/zu/src -I$(top_srcdir)/zm/src \
	-I$(top_srcdir)/zt/src -I$(top_srcdir)/ze/src \
	-DZI_EXPORTS @Z_CPPFLAGS@
AM_CXXFLAGS = @Z_CXXFLAGS@
AM_LDFLAGS = @Z_LDFLAGS@ @Z_SO_LDFLAGS@
include_HEADERS = ZiDir.hpp ZiFile.hpp ZiIP.hpp ZiLib.hpp ZiModule.hpp \
	ZiMultiplex.hpp ZiPlatform.hpp ZiSocket.hpp ZiRing.hpp
if NETLINK
include_HEADERS += ZiNetlinkMsg.hpp ZiNetlink.hpp zi_netlink.h
endif
lib_LTLIBRARIES = libZi.la
libZi_la_SOURCES = \
	ZiDir.cpp ZiFile.cpp ZiIP.cpp ZiLib.cpp ZiModule.cpp ZiMultiplex.cpp \
	ZiPlatform.cpp ZiRing.cpp
if NETLINK
libZi_la_SOURCES += ZiNetlink.cpp ZiNetlinkMsg.cpp
endif
libZi_la_LIBADD = $(top_builddir)/ze/src/libZe.la \
	$(top_builddir)/zt/src/libZt.la \
	$(top_builddir)/zm/src/libZm.la $(top_builddir)/zu/src/libZu.la \
	@Z_IO_LIBS@ @Z_ZT_LIBS@ @Z_MT_LIBS@
