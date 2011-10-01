# -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.65])
AC_INIT([admirable], [0.001], [BUG-REPORT-ADDRESS])
AC_CONFIG_SRCDIR([src/ranker-learn.cc])
AM_INIT_AUTOMAKE #([-Wall -Werror foreign])
AC_CONFIG_HEADERS([config.h])
AC_MSG_CHECKING(whether to enable debugging)
debug_default="yes"

AC_ARG_ENABLE(debug, [--enable-debug=[no/yes] turn on debugging
                     [default=$debug_default]],, enable_debug=$debug_default)

if test "x$enable_debug" = "xyes"; then
    CXXFLAGS="$CXXFLAGS -g -DDEBUG -Wall -Wextra -pedantic -std=c++0x"
    AC_MSG_RESULT(yes)
else
    CXXFLAGS="$CXXFLAGS -O6 -DNDEBUG -Wall -Wextra -pedantic -std=c++0x"
    AC_MSG_RESULT(no)
fi

boost_default="no"

AC_ARG_ENABLE(boost, [--enable-boost=[no/yes] turn on debugging
                     [default=$boost_default]],, enable_boost=$boost_default)

if test "x$enable_debug" = "xyes"; then
    AX_BOOST_BASE([1.41])
    AX_BOOST_SYSTEM
    AX_BOOST_THREAD
    LDFLAGS="$LDFLAGS  $(BOOST_LDFLAGS) $(BOOST_THREAD_LIB)"
    CXXFLAGS="$CXXFLAGS $(BOOST_CPPFLAGS) -DUSE_BOOST_THREAD"
    AM_CONDITIONAL([WANT_BOOST_THREAD],[ true])
    AC_MSG_RESULT(yes)
else
    CXXFLAGS="$CXXFLAGS"
    AC_MSG_RESULT(no)
fi



# libtool
#AC_PROG_LIBTOOL
#AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
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
