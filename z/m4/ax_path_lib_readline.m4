dnl @synopsis AX_PATH_LIB_READLINE [(A/NA)]
dnl
dnl check for readline lib and set READLINE_LIBS, READLINE_LDFLAGS,
dnl READLINE_CPPFLAGS
dnl
dnl also provide --with-readline option that may point to the $prefix of
dnl the readline installation - the macro will check $readline/include and
dnl $readline/lib to contain the necessary files.
dnl
dnl the usual two ACTION-IF-FOUND / ACTION-IF-NOT-FOUND are supported
dnl and they can take advantage of the LDFLAGS/CPPFLAGS additions.

dnl #ifdef HAVE_LIBREADLINE
dnl #  if defined(HAVE_READLINE_READLINE_H)
dnl #    include <readline/readline.h>
dnl #  elif defined(HAVE_READLINE_H)
dnl #    include <readline.h>
dnl #  else
dnl extern char *readline();
dnl #  endif
dnl #else
dnl /* no readline */
dnl #endif /* HAVE_LIBREADLINE */
dnl
dnl #ifdef HAVE_READLINE_HISTORY
dnl #  if defined(HAVE_READLINE_HISTORY_H)
dnl #    include <readline/history.h>
dnl #  elif defined(HAVE_HISTORY_H)
dnl #    include <history.h>
dnl #  else
dnl extern void add_history();
dnl extern int write_history();
dnl extern int read_history();
dnl #  endif
dnl #else
dnl /* no history */
dnl #endif /* HAVE_READLINE_HISTORY */

AC_DEFUN([AX_PATH_LIB_READLINE], [
  AC_MSG_CHECKING([lib readline])
  AC_ARG_WITH(readline,
              [AS_HELP_STRING([--with-readline=prefix],
			      [override location of readline library])],,
	      with_readline="yes")
  if test ".$with_readline" = ".no" ; then
    AC_MSG_RESULT([disabled])
    m4_ifval($2,$2)
  else
    AC_MSG_RESULT([(testing)])
    OLDCPPFLAGS="$CPPFLAGS"
    OLDLDFLAGS="$LDFLAGS"
    OLDLIBS="$LIBS"
    if test ".$with_readline" != "." -a ".$with_readline" != ".yes"; then
      CPPFLAGS="-I$with_readline/include $CPPFLAGS"
      LDFLAGS="-L$with_readline/lib -L$with_readline/lib64 -Wl,-R,$with_readline/lib -Wl,-R,$with_readline/lib64 $LDFLAGS"
    fi
    for readline_lib in readline edit editline; do
      for termcap_lib in "" termcap curses ncurses; do
	if test -z "$termcap_lib"; then
	  TRYLIBS="-l$readline_lib"
	else
	  TRYLIBS="-l$readline_lib -l$termcap_lib"
	fi
	LIBS="$OLDLIBS $TRYLIBS"
	AC_TRY_LINK_FUNC(readline, ax_cv_lib_readline="$TRYLIBS")
	if test -n "$ax_cv_lib_readline"; then
	  break
	fi
      done
      if test -n "$ax_cv_lib_readline"; then
	break
      fi
    done
    if test -z "$ax_cv_lib_readline"; then
      ax_cv_lib_readline="no"
    fi
    if test "$ax_cv_lib_readline" != "no"; then
      AC_DEFINE(HAVE_LIBREADLINE, 1,
		[Define if you have a readline compatible library])
      AC_CHECK_HEADERS(readline.h readline/readline.h)
      AC_TRY_LINK_FUNC(add_history, ax_cv_lib_readline_history="yes")
      if test "$ax_cv_lib_readline_history" = "yes"; then
	AC_DEFINE(HAVE_READLINE_HISTORY, 1,
		  [Define if your readline library has add_history])
	AC_CHECK_HEADERS(history.h readline/history.h)
      fi
    fi
    CPPFLAGS="$OLDCPPFLAGS"
    LDFLAGS="$OLDLDFLAGS"
    LIBS="$OLDLIBS"
    if test "$ax_cv_lib_readline" != "no"; then
      READLINE_LIBS="$ax_cv_lib_readline"
      READLINE_LDFLAGS=""
      READLINE_CPPFLAGS=""
      if test ".$with_readline" != "." -a ".$with_readline" != ".yes"; then
	AC_MSG_NOTICE([READLINE_LDFLAGS=-L$with_readline/lib -L$with_readline/lib64])
	READLINE_LDFLAGS="-L$with_readline/lib -L$with_readline/lib64 -Wl,-R,$with_readline/lib -Wl,-R,$with_readline/lib64"
	if test -d "$with_readline/include"; then
	  AC_MSG_NOTICE([READLINE_CPPFLAGS=-I$with_readline/include])
	  READLINE_CPPFLAGS="-I$with_readline/include"
	fi
      fi
      AC_MSG_CHECKING([lib readline])
      AC_MSG_RESULT([$READLINE_LIBS])
      m4_ifval($1,$1)
    else
      AC_MSG_CHECKING([lib readline])
      AC_MSG_RESULT([no])
      m4_ifval($2,$2)
    fi
  fi
])
