dnl @synopsis AX_PATH_LIB_PCRE [(A/NA)]
dnl
dnl check for pcre lib and set PCRE_LDFLAGS, PCRE_CPPFLAGS
dnl
dnl also provide --with-pcre option that may point to the $prefix of
dnl the pcre installation - the macro will check $pcre/include and
dnl $pcre/lib to contain the necessary files.
dnl
dnl the usual two ACTION-IF-FOUND / ACTION-IF-NOT-FOUND are supported
dnl and they can take advantage of the LDFLAGS/CPPFLAGS additions.
dnl 
dnl modified to check for pcre_compile2 by djnz00@gmail.com 2008-12-12
dnl
dnl @category InstalledPackages
dnl @author Guido U. Draheim <guidod@gmx.de>
dnl @version 2006-10-13
dnl @license GPLWithACException

AC_DEFUN([AX_PATH_LIB_PCRE], [
  AC_MSG_CHECKING([lib pcre])
  AC_ARG_WITH(pcre, [AS_HELP_STRING([--with-pcre=prefix],
				    [override location of pcre library])],,
	      with_pcre="yes")
  if test ".$with_pcre" = ".no" ; then
    AC_MSG_RESULT([disabled])
    m4_ifval($2,$2)
  else
    AC_MSG_RESULT([(testing)])
    OLDLIBS="$LIBS"
    OLDCPPFLAGS="$CPPFLAGS"
    OLDLDFLAGS="$LDFLAGS"
    if test ".$with_pcre" != "." -a ".$with_pcre" != ".yes"; then
      CPPFLAGS="-I$with_pcre/include $CPPFLAGS"
      LDFLAGS="-L$with_pcre/lib -L$with_pcre/lib64 -Wl,-R,$with_pcre/lib -Wl,-R,$with_pcre/lib64 $LDFLAGS"
    fi
    AC_CHECK_LIB(pcre, pcre_compile2)
    LIBS="$OLDLIBS"
    CPPFLAGS="$OLDCPPFLAGS"
    LDFLAGS="$OLDLDFLAGS"
    if test "$ac_cv_lib_pcre_pcre_compile2" = "yes"; then
      PCRE_LIBS="-lpcre"
      PCRE_LDFLAGS=""
      PCRE_CPPFLAGS=""
      if test ".$with_pcre" != "." -a ".$with_pcre" != ".yes"; then
	AC_MSG_NOTICE([PCRE_LDFLAGS=-L$with_pcre/lib -L$with_pcre/lib64])
	PCRE_LDFLAGS="-L$with_pcre/lib -L$with_pcre/lib64 -Wl,-R,$with_pcre/lib -Wl,-R,$with_pcre/lib64"
	if test -d "$with_pcre/include"; then
	  AC_MSG_NOTICE([PCRE_CPPFLAGS=-I$with_pcre/include])
	  PCRE_CPPFLAGS="-I$with_pcre/include"
	fi
      fi
      AC_MSG_CHECKING([lib pcre])
      AC_MSG_RESULT([$PCRE_LIBS])
      m4_ifval($1,$1)
    else
      AC_MSG_CHECKING([lib pcre])
      AC_MSG_RESULT([no])
      m4_ifval($2,$2)
    fi
  fi
])
