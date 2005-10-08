AC_DEFUN([VS_CONFIG_VSLAUNCHER], 
[CHECK_GTK=1
HAVE_GTK=no
AC_ARG_ENABLE(gtk, AC_HELP_STRING([--disable-gtk], [Disable building of
vslauncher (disable GTK)]), [CHECK_GTK=0])
if (test x$CHECK_GTK = x1); then
AM_PATH_GTK([1.2.0], [HAVE_GTK=yes], [HAVE_GTK=no])
fi
AM_CONDITIONAL([VS_MAKE_LAUNCHER], [test x$HAVE_GTK = xyes]) 
if test x$HAVE_GTK = xno; then
  if test x$CHECK_GTK = x1; then
    AC_MSG_WARN([[GTK Was not found.  VSLAUNCHER will not be built.]])
  fi
fi
])
