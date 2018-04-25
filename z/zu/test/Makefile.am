METASOURCES = AUTO
AM_CPPFLAGS = -I$(top_srcdir)/zu/src \
	@Z_CPPFLAGS@
AM_CXXFLAGS = @Z_CXXFLAGS@
AM_LDFLAGS = @Z_LDFLAGS@
LDADD = $(top_builddir)/zu/src/libZu.la
bin_PROGRAMS = \
	ZuConversionTest ZuHashTest ZuCmpTest ZuBoxTest ZuNetworkTest \
	ZuCanTest ZuArrayTest ZuPairTest ZuIDTest
ZuConversionTest_SOURCES = ZuConversionTest.cpp
ZuHashTest_SOURCES = ZuHashTest.cpp Analyze.hpp
ZuCmpTest_SOURCES = ZuCmpTest.cpp
ZuBoxTest_SOURCES = ZuBoxTest.cpp
ZuNetworkTest_SOURCES = ZuNetworkTest.cpp
ZuCanTest_SOURCES = ZuCanTest.cpp
ZuArrayTest_SOURCES = ZuArrayTest.cpp
ZuPairTest_SOURCES = ZuPairTest.cpp
ZuIDTest_SOURCES = ZuIDTest.cpp
