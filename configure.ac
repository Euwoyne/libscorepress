# Process this file with autoconf to produce a configure script.

# initialize package
AC_PREREQ([2.69])
AC_INIT([libscorepress], [0.2.5])
AC_CONFIG_SRCDIR([src/engraver.cpp])
AC_CONFIG_HEADERS([include/scorepress/config.hh:include/scorepress/config.hh.ac])
AC_LANG([C++])

AC_SUBST([PACKAGE_VERSION_MAIN], [0])

AC_DEFINE([LIBSCOREPRESS_PACKAGE_NAME], ["AC_PACKAGE_NAME"])
AC_DEFINE([LIBSCOREPRESS_VERSION_STRING], ["AC_PACKAGE_VERSION"])
AC_DEFINE([LIBSCOREPRESS_VERSION_MAJOR], [0])
AC_DEFINE([LIBSCOREPRESS_VERSION_MINOR], [2])
AC_DEFINE([LIBSCOREPRESS_VERSION_PATCH], [5])

# save user flags
CFLAGS_ORI="$CFLAGS"
LDFLAGS_ORI="$LDFLAGS"
LIBS_ORI="$LIBS"

# check commandline arguments
#AC_PRESERVE_HELP_ORDER
AC_ARG_ENABLE([doxygen],
            [AS_HELP_STRING([--enable-doxygen], [enable 'make doc' with doxygen])],
            [with_doxygen="$withval"],
            [with_doxygen="no"])
AC_ARG_ENABLE([fraction-check],
            [AS_HELP_STRING([--enable-fraction-check], [enable overflow check for rational numbers])],
            [with_sizecheck="$withval"],
            [with_sizecheck=no])
AC_ARG_ENABLE([debug],
            [AS_HELP_STRING([--enable-debug], [enable debugging with GDB])],
            [with_debug="$withval"],
            [with_debug="no"])
AC_ARG_ENABLE([profile],
            [AS_HELP_STRING([--enable-profile], [enable profiling with GProf])],
            [with_profile="$withval"],
            [with_profile="no"])
AC_ARG_WITH([libxml2],
            [AS_HELP_STRING([--with-libxml2], [location of the libxml2 headers [/usr/include/libxml2]])],
            [with_libxml2="$withval"],
            [with_libxml2="auto"])

AS_IF([test "x$with_sizecheck" != "xno"],
    AC_SUBST([FRACT_SIZECHECK], [\${SIZECHECK_ON}]),
    AC_SUBST([FRACT_SIZECHECK], [\${SIZECHECK_OFF}]))

AS_IF([test "x$with_debug" != "xno"],
    AC_DEFINE([LIBSCOREPRESS_DEBUG], [1]),
    AC_DEFINE([LIBSCOREPRESS_DEBUG], [0]))
AC_SUBST([MODE], [RELEASE])

AS_IF([test "x$with_profile" != "xno"],
    AC_SUBST([MODE], [PROFILE]))

AS_IF([test "x$with_debug" != "xno"],
    AC_SUBST([MODE], [DEBUG]))

# check for programs
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PATH_PROG([AR], [ar])
AC_PATH_PROG([LDCONFIG], [ldconfig])
AC_PATH_PROG([PKGCONFIG], [pkg-config])

AS_IF([test "x$with_doxygen" != "xno"],
      [AC_SUBST([DOXYGEN], [$with_doxygen])]
      [AC_SUBST([DOXYGEN], [:])])
AS_IF([test -z "$with_doxygen"],
      [AC_PATH_PROG([DOXYGEN], [doxygen])])
AS_IF([test "x$with_doxygen" != "xno"],
      [AC_PATH_PROG([SED], [sed])])
AS_IF([test "x$with_doxygen" != "xno"],
      [AC_PATH_PROG([LATEX], [pdflatex])])
AS_IF([test "x$with_doxygen" != "xno"],
      [AC_PATH_PROG([MAKEINDEX], [makeindex])])
if test -z "$DOXYGEN";   then AC_SUBST([DOXYGEN],   [:]) fi
if test -z "$SED";       then AC_SUBST([SED],       [:]) fi
if test -z "$LATEX";     then AC_SUBST([LATEX],     [:]) fi
if test -z "$MAKEINDEX"; then AC_SUBST([MAKEINDEX], [:]) fi

# check libraries
#   libxml2
if test "x$with_libxml2" != "xauto"; then
    AC_SUBST([LIBXML_CFLAGS], [-I$with_libxml2])
    AC_SUBST([LIBXML_LIBS], [-lxml2])
elif test -n "$PKGCONFIG" -a `$PKGCONFIG --exists libxml-2.0; echo $?` -eq 0; then
    AC_SUBST([LIBXML_CFLAGS], [`\${PKGCONFIG} libxml-2.0 --cflags`])
    AC_SUBST([LIBXML_LIBS], [`\${PKGCONFIG} libxml-2.0 --libs`])
else
    AC_SUBST([LIBXML_CFLAGS], [-I/usr/include/libxml2])
    AC_SUBST([LIBXML_LIBS], [-lxml2])
fi

CFLAGS="$CFLAGS_ORI $LIBXML_CFLAGS"
LDFLAGS="$LDFLAGS_ORI $LIBXML_LIBS"
LIBS="$LIBS_ORI $LIBXML_LIBS"
AC_CHECK_LIB([xml2], [xmlReaderForFile],, AC_MSG_ERROR([cannot find library 'libxml2']))

# check headers
AC_CHECK_HEADER([cstdlib],,  AC_MSG_ERROR([Cannot find <cstdlib> header.]))
AC_CHECK_HEADER([cstddef],,  AC_MSG_ERROR([Cannot find <cstddef> header.]))
AC_CHECK_HEADER([cmath],,    AC_MSG_ERROR([Cannot find <cmath> header.]))
AC_CHECK_HEADER([climits],,  AC_MSG_ERROR([Cannot find <climits> header.]))
AC_CHECK_HEADER([cstring],,  AC_MSG_ERROR([Cannot find <cstring> header.]))
AC_CHECK_HEADER([iostream],, AC_MSG_ERROR([Cannot find <iostream> header.]))
AC_CHECK_HEADER([string],,   AC_MSG_ERROR([Cannot find <string> header.]))
AC_CHECK_HEADER([vector],,   AC_MSG_ERROR([Cannot find <vector> header.]))
AC_CHECK_HEADER([list],,     AC_MSG_ERROR([Cannot find <list> header.]))
AC_CHECK_HEADER([deque],,    AC_MSG_ERROR([Cannot find <deque> header.]))
AC_CHECK_HEADER([map],,      AC_MSG_ERROR([Cannot find <map> header.]))
AC_CHECK_HEADER([set],,      AC_MSG_ERROR([Cannot find <set> header.]))

CPPFLAGS="$CFLAGS_ORI $LIBXML_CFLAGS"
CFLAGS="$CFLAGS_ORI $LIBXML_CFLAGS"
AC_CHECK_HEADER([libxml/xmlreader.h],, AC_MSG_ERROR([Failed to find <xmlreader.h>]))

# check types
AS_IF([test "x$with_sizecheck" != "xno"],
    AC_TYPE_LONG_LONG_INT)
AC_TYPE_SIZE_T

# check functions
CFLAGS="$CFLAGS_ORI $LIBXML_CFLAGS"
LDFLAGS="$LDFLAGS_ORI $LIBXML_LIBS"
LIBS="$LIBS_ORI $LIBXML_LIBS"
AC_CHECK_FUNC([xmlReaderForFile],, AC_MSG_ERROR([library 'libxml2' does not provide the 'xmlReader' interface]))
AC_CHECK_FUNC([xmlReaderForMemory],, AC_MSG_ERROR([library 'libxml2' does not provide the 'xmlReader' interface]))

# generate output
AC_SUBST([CFLAGS],[$CFLAGS_ORI])
AC_SUBST([LDFLAGS],[$LDFLAGS_ORI])
AC_SUBST([LIBS],[$LIBS_ORI])
AC_CONFIG_FILES([Makefile:Makefile.ac])
AC_OUTPUT()

