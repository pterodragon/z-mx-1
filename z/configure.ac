m4_define([Z_VERSION_], m4_esyscmd([grep Z_VERNAME version.h | sed 's/[^0-9\.]//g;1q' | tr -d '\n']))
AC_INIT(zlib, Z_VERSION_)
Z_VERSION=Z_VERSION_

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS(config.h)

AM_INIT_AUTOMAKE

AC_LANG_CPLUSPLUS
AC_PROG_CXX
AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL

AX_PATH_LIB_CK(, [AC_MSG_ERROR([libck required])])
CPPFLAGS="$CK_CPPFLAGS $CPPFLAGS"
LDFLAGS="$CK_LDFLAGS $LDFLAGS"

AX_PATH_LIB_HWLOC(, [AC_MSG_ERROR([libhwloc required])])
CPPFLAGS="$HWLOC_CPPFLAGS $CPPFLAGS"
LDFLAGS="$HWLOC_LDFLAGS $LDFLAGS"

AX_PATH_LIB_PCRE(, [AC_MSG_ERROR([libpcre required])])
CPPFLAGS="$PCRE_CPPFLAGS $CPPFLAGS"
LDFLAGS="$PCRE_LDFLAGS $LDFLAGS"

AC_MSG_CHECKING([system multithreading libraries])
case $build_os in
  linux-*) 
    Z_CXXFLAGS=""
    Z_LDFLAGS="-rdynamic"
    Z_SO_LDFLAGS="-release $Z_VERSION"
    Z_MT_LIBS="$CK_LIBS $HWLOC_LIBS -lpthread -lrt"
    Z_ZT_LIBS="$PCRE_LIBS"
    Z_IO_LIBS="-ldl"
    ;;
  mingw*)
    mingw=yes
    case $build_cpu in
      i686) mingw32=yes;;
      x86_64) mingw64=yes;;
      *) ;;
    esac
    Z_CXXFLAGS="-mthreads"
    Z_LDFLAGS="-Wl,--enable-auto-import"
    Z_SO_LDFLAGS="-no-undefined"
    Z_MT_LIBS="$CK_LIBS $HWLOC_LIBS"
    Z_ZT_LIBS="$PCRE_LIBS -liconv"
    Z_IO_LIBS="-lws2_32 -lwsock32"
    ;;
  *)
    Z_CXXFLAGS=""
    Z_LDFLAGS=""
    Z_SO_LDFLAGS=""
    Z_MT_LIBS="$CK_LIBS $HWLOC_LIBS -lpthread"
    Z_ZT_LIBS="$PCRE_LIBS"
    Z_IO_LIBS="-ldl"
    ;;
esac
AC_MSG_RESULT([${Z_MT_LIBS}])

AX_C_VA_LIST_IS_ARRAY

AX_PATH_JNI([jni="yes"], [dnl
  AC_MSG_NOTICE([JNI unavailable])
  jni="no"
])

AX_PATH_LIB_PCAP(, [AC_MSG_ERROR([libpcap required])])
CPPFLAGS="$PCAP_CPPFLAGS $CPPFLAGS"
LDFLAGS="$PCAP_LDFLAGS $LDFLAGS"

AX_PATH_LIB_LZ4(, [AC_MSG_ERROR([liblz4 required])])
CPPFLAGS="$LZ4_CPPFLAGS $CPPFLAGS"
LDFLAGS="$LZ4_LDFLAGS $LDFLAGS"

AX_PATH_LIB_MBEDTLS(, [AC_MSG_ERROR([libmbedtls required])])
CPPFLAGS="$MBEDTLS_CPPFLAGS $CPPFLAGS"
LDFLAGS="$MBEDTLS_LDFLAGS $LDFLAGS"

AX_PATH_LIB_FLATBUFFERS(, [AC_MSG_ERROR([libflatbuffers required])])
CPPFLAGS="$FLATBUFFERS_CPPFLAGS $CPPFLAGS"
LDFLAGS="$FLATBUFFERS_LDFLAGS $LDFLAGS"

AX_PATH_LIB_CPPCODEC(, [AC_MSG_ERROR([libcppcodec required])])
CPPFLAGS="$CPPCODEC_CPPFLAGS $CPPFLAGS"

if test ".$mingw" != ".yes"; then
  AC_ARG_ENABLE([netlink],
    [  --enable-netlink        enable netlink],
    [case "${enableval}" in
      yes) netlink=true ;;
      no) netlink=false ;;
      *) AC_MSG_ERROR([bad value ${enableval} for --enable-netlink]) ;;
    esac], [netlink=false])
fi
AM_CONDITIONAL([NETLINK], [test x$netlink = xtrue])

AX_PATH_LIB_READLINE(, [AC_MSG_ERROR([libreadline required])])
CPPFLAGS="$READLINE_CPPFLAGS $CPPFLAGS"
LDFLAGS="$READLINE_LDFLAGS $LDFLAGS"

PKG_CHECK_MODULES([GTK], [gtk+-3.0 >= 3.24.0], [dnl
  gtk="yes"
], [dnl
  AC_MSG_NOTICE([Gtk unavailable])
  gtk="no"
])
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LDFLAGS)
AC_SUBST(GTK_LIBS)

Z_CPPFLAGS="-D_REENTRANT -D_GNU_SOURCE"

AM_CONDITIONAL([GXX], [test ".$GXX" = ".yes"])
AM_CONDITIONAL([MINGW], [test ".$mingw" = ".yes"])
AM_CONDITIONAL([MINGW32], [test ".$mingw32" = ".yes"])
AM_CONDITIONAL([MINGW64], [test ".$mingw64" = ".yes"])
AM_CONDITIONAL([JNI], [test ".$jni" = ".yes"])
AM_CONDITIONAL([GTK], [test ".$gtk" = ".yes"])

if test ".$GXX" = ".yes"; then
  Z_CXXFLAGS="$Z_CXXFLAGS -Wall -Wno-parentheses -Wno-invalid-offsetof -Wno-misleading-indentation -fstrict-aliasing"
fi

if test ".$MX_CLANG" = ".yes"; then
  Z_CXXFLAGS="$Z_CXXFLAGS -Wno-unused-local-typedef -Wno-empty-body -Wno-c++11-extensions"
fi

AC_CHECK_PROG(PERL, "perl")

CPPFLAGS=`echo $CPPFLAGS | perl -pe 's/-I(@<:@^ \n@:>@+)(@<:@ \n@:>@)/$I{$1}++ ? "" : "-I${1}${2}"/ge;'`
LDFLAGS=`echo $LDFLAGS | perl -pe 's/-L(@<:@^ \n@:>@+)(@<:@ \n@:>@)/$L{$1}++ ? "" : "-L${1}${2}"/ge; s/(-Wl,)?-R,(@<:@^ \n@:>@+)(@<:@ \n@:>@)/$R{$2}++ ? "" : "${1}-R,${2}${3}"/ge'`

AC_SUBST(Z_CPPFLAGS)
AC_SUBST(Z_CXXFLAGS)
AC_SUBST(Z_LDFLAGS)
AC_SUBST(Z_SO_LDFLAGS)
AC_SUBST(Z_MT_LIBS)
AC_SUBST(Z_ZT_LIBS)
AC_SUBST(Z_IO_LIBS)

AC_SUBST(JNI_CPPFLAGS)
AC_SUBST(PCAP_LIBS)
AC_SUBST(LZ4_LIBS)
AC_SUBST(MBEDTLS_LIBS)
AC_SUBST(FLATBUFFERS_LIBS)
AC_SUBST(READLINE_LIBS)

for i in zu zm zjni zt ze zi ztls zfb zv zp zdb zrl zgtk
do ( cd $i/src; test ! -e AC_PACKAGE_NAME && ln -s . AC_PACKAGE_NAME )
done

AC_OUTPUT(Makefile \
	zu/Makefile zu/src/Makefile zu/test/Makefile \
	zm/Makefile zm/src/Makefile zm/test/Makefile \
	zjni/Makefile zjni/src/Makefile \
	zt/Makefile zt/src/Makefile zt/test/Makefile \
	ze/Makefile ze/src/Makefile ze/test/Makefile \
	zi/Makefile zi/src/Makefile zi/test/Makefile \
	ztls/Makefile ztls/src/Makefile ztls/test/Makefile \
	zfb/Makefile zfb/src/Makefile zfb/test/Makefile \
	zv/Makefile zv/src/Makefile zv/test/Makefile \
	zp/Makefile zp/src/Makefile \
	zdb/Makefile zdb/src/Makefile zdb/test/Makefile \
	zrl/Makefile zrl/src/Makefile zrl/test/Makefile \
	zgtk/Makefile zgtk/src/Makefile zgtk/test/Makefile)
