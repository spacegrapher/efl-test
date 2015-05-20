#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([efl-test], [0.1], [https://github.com/spacegrapher/efl-test])
AM_INIT_AUTOMAKE([no-define])
AM_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET

# Checks for libraries.
PKG_CHECK_MODULES(ELEMENTARY, [elementary])
AC_SUBST(ELEMENTARY_CFLAGS)
AC_SUBST(ELEMENTARY_LIBS)

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h sys/time.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([gettimeofday memset pow sqrt])

AC_CONFIG_FILES([Makefile
		 src/Makefile
                 src/opengles1/Makefile
		 src/opengles2/Makefile
		 src/opengles3/Makefile])
AC_OUTPUT
