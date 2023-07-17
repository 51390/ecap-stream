AC_INIT([ecap-stream], [0.1])
AC_CONFIG_SRCDIR([src])
AM_INIT_AUTOMAKE([-Wall foreign])
LT_INIT
AC_PROG_CXX
AC_CONFIG_FILES([Makefile src/Makefile tests/Makefile])
PKG_CHECK_MODULES(libecap, [libecap >= 1.0 libecap < 1.1])
PKG_CHECK_MODULES(check, [check >= 0.15.2])
AC_SUBST([CHECK_CFLAGS])
AC_SUBST([CHECK_LIBS])

AC_OUTPUT
