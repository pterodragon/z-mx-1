METASOURCES = AUTO
AM_CPPFLAGS = -I$(top_srcdir)/zu/src -I$(top_srcdir)/zlz4/src @Z_CPPFLAGS@
AM_CXXFLAGS = @Z_CXXFLAGS@
AM_LDFLAGS = @Z_LDFLAGS@
LDADD = $(top_builddir)/zlz4/src/libZlz4.la $(top_builddir)/zu/src/libZu.la
noinst_PROGRAMS = Zlz4Test
Zlz4Test_SOURCES = Zlz4Test.cpp
