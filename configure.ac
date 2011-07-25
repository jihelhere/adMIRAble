# -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.65])
AC_INIT([admirable], [0.001], [BUG-REPORT-ADDRESS])
AC_CONFIG_SRCDIR([src/ranker-learn-zcat.cc])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_HEADERS([config.h])

# libtool
AC_PROG_LIBTOOL
LT_INIT
AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
CXXFLAGS=""
AC_PROG_CXX
AC_PROG_CC

AC_SUBST(CXXFLAGS)
AC_SUBST(LDFLAGS)

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_TYPE_PID_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_STRTOD
AC_CHECK_FUNCS([dup2 strchr strdup strrchr strstr strtol])

AC_OUTPUT(Makefile src/Makefile)
