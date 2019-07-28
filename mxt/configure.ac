m4_define([MXT_VERSION_], m4_esyscmd([grep MXT_VERNAME version.h | sed 's/[^0-9\.]//g;1q' | tr -d '\n']))
AC_INIT(MxT, MXT_VERSION_)
MXT_VERSION=MXT_VERSION_

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS(config.h)

AM_INIT_AUTOMAKE

AC_LANG_CPLUSPLUS
AC_PROG_CXX
AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL

AC_MSG_CHECKING([which threading environment to use])
case $build_os in
	linux-*) 
		AC_MSG_RESULT([-lpthread -lrt])
		MXT_LDFLAGS="-rdynamic"
		MXT_SO_LDFLAGS="-release $MXT_VERSION"
		MXT_XLIBS="-lhwloc -ldl -lpthread -lrt"
		;;
	mingw*)
		AC_MSG_RESULT([mingw])
		MXT_LDFLAGS="-Wl,--enable-auto-import"
		MXT_SO_LDFLAGS="-no-undefined"
		MXT_XLIBS="-lws2_32 -lwsock32 -liconv"
		;;
	*)
		AC_MSG_RESULT([default])
		MXT_LDFLAGS=""
		MXT_SO_LDFLAGS=""
		MXT_XLIBS=""
		;;
esac

AX_PATH_LIB_Z(8, 5, [-lZv -lZi -lZe -lZt -lZm -lZu],,
	      [AC_MSG_ERROR([ZLib >= 8.4 required])])

AC_SUBST(Z_CPPFLAGS)
AC_SUBST(Z_CXXFLAGS)
AC_SUBST(Z_LDFLAGS)
AC_SUBST(Z_LIBS)

AX_PATH_LIB_MXBASE(4, 3, [-lMxBase],, [AC_MSG_ERROR([MxBase >= 4.2 required])])

AC_SUBST(MXBASE_CPPFLAGS)
AC_SUBST(MXBASE_LDFLAGS)
AC_SUBST(MXBASE_LIBS)

MXT_CPPFLAGS=""

AC_SUBST(MXT_CPPFLAGS)
AC_SUBST(MXT_LDFLAGS)
AC_SUBST(MXT_SO_LDFLAGS)
AC_SUBST(MXT_XLIBS)

AC_OUTPUT(Makefile src/Makefile test/Makefile)
