# Process this file with autoconf to produce a configure script.
#
#  ScorePress - Music Engraving Software  (libscorepress)
#  Copyright (C) 2014 Dominik Lehmann
#  
#  Licensed under the EUPL, Version 1.1 or - as soon they
#  will be approved by the European Commission - subsequent
#  versions of the EUPL (the "Licence");
#  You may not use this work except in compliance with the
#  Licence.
#  
#  Unless required by applicable law or agreed to in
#  writing, software distributed under the Licence is
#  distributed on an "AS IS" basis, WITHOUT WARRANTIES OR
#  CONDITIONS OF ANY KIND, either expressed or implied.
#  See the Licence for the specific language governing
#  permissions and limitations under the Licence.
#

# initialize package
AC_PREREQ([2.69])
AC_INIT([libscorepress], [0.2.6])
AC_CONFIG_SRCDIR([src/engraver.cpp])
AC_CONFIG_HEADERS([config.hh:include/scorepress/config.hh.ac])
AC_CONFIG_FILES([Makefile:Makefile.ac data/scorepress.pc:data/scorepress.pc.ac])
AC_LANG([C++])

AC_SUBST([PACKAGE_VERSION_MAIN], [0])

AC_DEFINE([LIBSCOREPRESS_PACKAGE_NAME], ["AC_PACKAGE_NAME"])
AC_DEFINE([LIBSCOREPRESS_VERSION_STRING], ["AC_PACKAGE_VERSION"])
AC_DEFINE([LIBSCOREPRESS_VERSION_MAJOR], [0])
AC_DEFINE([LIBSCOREPRESS_VERSION_MINOR], [2])
AC_DEFINE([LIBSCOREPRESS_VERSION_PATCH], [6])

# save user flags
CPPFLAGS_ORI="$CPPFLAGS"
CXXFLAGS_ORI="$CXXFLAGS"
LDFLAGS_ORI="$LDFLAGS"
LIBS_ORI="$LIBS"

# check commandline arguments
AC_ARG_ENABLE([debug],          [AS_HELP_STRING([--disable-debug],         [disable creation of debugging symbols])])
AC_ARG_ENABLE([profile],        [AS_HELP_STRING([--enable-profile],        [enable profiling with GProf])])
AC_ARG_ENABLE([fraction-check], [AS_HELP_STRING([--enable-fraction-check], [enable overflow check for rational numbers])])
AC_ARG_ENABLE([rpath],          [AS_HELP_STRING([--enable-rpath],          [enable use of the linkers -rpath flag])])
AC_ARG_WITH(  [doxygen],        [AS_HELP_STRING([--with-doxygen],          [enable documentation compilation with doxygen])])
AC_ARG_VAR(   [RPATH],          [add the given directory to the runtime library search path])

AC_SUBST([MODE], [RELEASE])

AS_IF([test "x$enable_debug" != "xno"],
      [AC_DEFINE([LIBSCOREPRESS_DEBUG], [1])],
      [AC_DEFINE([LIBSCOREPRESS_DEBUG], [0])])
AS_IF([test "x$enable_debug" != "xno"],
      [AC_SUBST([MODE], [DEBUG])])
AS_IF([test "x$enable_profile" = "xyes"],
      [AC_SUBST([MODE], [PROFILE])])
AS_IF([test "x$enable_fraction_check" = "xyes"],
      [AC_SUBST([FRACT_SIZECHECK], [\${SIZECHECK_ON}])],
      [AC_SUBST([FRACT_SIZECHECK], [\${SIZECHECK_OFF}])])

AC_SUBST([RPATH], [$RPATH])

# get host machine
AC_CANONICAL_HOST
case "$host_os" in
    linux*|mingw* ) ;;
    * ) AC_MSG_WARN([unrecognized OS '$host_os']) ;;
esac

# check shell
AS_IF([test "x$SHELL" != "x"],
      [AC_SUBST([SH], [$SHELL])],
      [AC_PATH_PROG([SH], [/bin/sh])])

# check compiler
AC_PROG_CXX([g++ gcc clang c++])

# set compiler specific flags
case "$CXX" in
gcc | g++ )
    # mode flags
    AC_CHECK_PROG([DEBUG], [gdb], [-ggdb], [-g])
    AC_SUBST([DEBUG],      ["$DEBUG -fno-inline -Werror"])
    AC_SUBST([PROFILE],    ["-O2 -fno-omit-frame-pointer"])
    AC_SUBST([RELEASE],    ["-Ofast"])
    
    # compile flags
    AC_SUBST([C_FLAGS],   [])
    AC_SUBST([C_DEBUG],   [])
    AC_SUBST([C_PROFILE], [])
    AC_SUBST([C_RELEASE], [])
    
    case "$host_os" in
        linux* ) AC_SUBST([C_FLAGS_SO], ["-fPIC"]) ;;
        * )      AC_SUBST([C_FLAGS_SO], []) ;;
    esac
    AC_SUBST([C_DEBUG_SO],   [])
    AC_SUBST([C_PROFILE_SO], ["-fvisibility=hidden -fvisibility-inlines-hidden"])
    AC_SUBST([C_RELEASE_SO], ["-fvisibility=hidden -fvisibility-inlines-hidden"])
    
    # linker flags
    AC_SUBST([LD_FLAGS], ["-fPIC -shared -lc -Wl,-soname,\${soname}"])
    AC_SUBST([LD_DEBUG], [])
    AC_SUBST([LD_PROFILE], [])
    AC_SUBST([LD_RELEASE], [])
    
    # general flags
    AC_SUBST([CTRLFLAGS],  ["-ansi -std=c++11 -pedantic -fno-nonansi-builtins"])
    AS_IF([test "x$enable_rpath" = "xyes"],
        [AC_SUBST([RPATHFLAG],  [])],
        [AC_SUBST([RPATHFLAG],  ["-Wl,-rpath,"])])
    AC_SUBST([WARNINGS],   ["-W                   \\
             -Wall                \\
             -Wextra              \\
             -Wcast-qual          \\
             -Wcast-align         \\
             -Wconversion         \\
             -Wfloat-equal        \\
             -Winit-self          \\
             -Wold-style-cast     \\
             -Wpointer-arith      \\
             -Wredundant-decls    \\
             -Wshadow             \\
             -Wundef              \\
             -Wunreachable-code   \\
             -Wwrite-strings      \\
             -Wno-unknown-pragmas \\
             -Wno-conversion"])
    
    # set compiler-version specific flags
    AS_IF([gcc_version=`$CXX -dumpversion 2> /dev/null`],[
            if test `echo -n $gcc_version | cut -d'.' -f1` -eq 4 &&
               test `echo -n $gcc_version | cut -d'.' -f2` -ge 8 ||
               test `echo -n $gcc_version | cut -d'.' -f1` -gt 4
            then
                AC_SUBST([DEBUG], ["$DEBUG -Og"])
            else
                AC_SUBST([DEBUG], ["$DEBUG -O0"])
            fi ])
    ;;
clang )
    # mode flags
    AC_SUBST([DEBUG],      ["-g -O0"])
    AC_SUBST([PROFILE],    ["-O2"])
    AC_SUBST([RELEASE],    ["-Ofast"])
    
    # compile flags
    AC_SUBST([C_FLAGS],   [])
    AC_SUBST([C_DEBUG],   [])
    AC_SUBST([C_PROFILE], [])
    AC_SUBST([C_RELEASE], [])
    
    case "$host_os" in
        linux* ) AC_SUBST([C_FLAGS_SO], ["-fPIC"]) ;;
        * )      AC_SUBST([C_FLAGS_SO], []) ;;
    esac
    AC_SUBST([C_DEBUG_SO],   [])
    AC_SUBST([C_PROFILE_SO], ["-fvisibility=hidden"])
    AC_SUBST([C_RELEASE_SO], ["-fvisibility=hidden"])
    
    # linker flags
    AC_SUBST([LD_FLAGS], ["-shared -lc -lm -lstdc++ -Wl,-soname,\${soname}"])
    AC_SUBST([LD_DEBUG], [])
    AC_SUBST([LD_PROFILE], [])
    AC_SUBST([LD_RELEASE], [])
    
    # general flags
    AC_SUBST([CTRLFLAGS],  ["-ansi -pedantic -fexceptions"])
    AS_IF([test "x$enable_rpath" = "xyes"],
        [AC_SUBST([RPATHFLAG],  [])],
        [AC_SUBST([RPATHFLAG],  ["-Wl,-rpath,"])])
    AC_SUBST([WARNINGS],   ["-Weverything              \\
             -Wno-padded                 \\
             -Wno-switch-enum            \\
             -Wno-documentation          \\
             -Wno-global-constructors    \\
             -Wno-exit-time-destructors  \\
             -Wno-sign-conversion        \\
             -Wno-conversion"])
    ;;
*)
    echo "WARNING: Unknown Compiler!"
    AC_SUBST([DEBUG],        [])
    AC_SUBST([PROFILE],      [])
    AC_SUBST([RELEASE],      [])
    AC_SUBST([C_FLAGS],      [])
    AC_SUBST([C_DEBUG],      [])
    AC_SUBST([C_PROFILE],    [])
    AC_SUBST([C_RELEASE],    [])
    AC_SUBST([C_FLAGS_SO],   [])
    AC_SUBST([C_DEBUG_SO],   [])
    AC_SUBST([C_PROFILE_SO], [])
    AC_SUBST([C_RELEASE_SO], [])
    AC_SUBST([LD_FLAGS],     [])
    AC_SUBST([LD_DEBUG],     [])
    AC_SUBST([LD_PROFILE],   [])
    AC_SUBST([LD_RELEASE],   [])
    AC_SUBST([CTRLFLAGS],    [])
    AC_SUBST([RPATHFLAG],    [])
    AC_SUBST([WARNINGS],     [])
esac

# check for programs
AC_PROG_LN_S
AC_PATH_PROG([LDCONFIG],  [ldconfig])
AC_PROG_INSTALL

AC_PATH_PROG([AR],        [ar])
AC_PATH_PROG([GZIP],      [gzip])
AC_PATH_PROG([BZIP2],     [bzip2])
AC_PATH_PROG([ZIP],       [zip])
AC_PATH_PROG([PKGCONFIG], [pkg-config])

AC_PATH_PROG([MAKEINFO],  [makeinfo])
AS_IF([test "x$with_doxygen" != "xno"],
      [AC_PATH_PROG([DOXYGEN],   [doxygen])
       AC_PATH_PROG([CHMOD],     [chmod])
       AC_PATH_PROG([LATEX],     [pdflatex])
       AC_PATH_PROG([MAKEINDEX], [makeindex])],
      [AC_SUBST([DOXYGEN],       [:])])

if test -z "$LDCONFIG";  then AC_SUBST([LDCONFIG],  [:]) fi

if test -z "$AR";        then AC_SUBST([AR],        [:]) fi
if test -z "$GZIP";      then AC_SUBST([GZIP],      [:]) fi
if test -z "$BZIP2";     then AC_SUBST([BZIP2],     [:]) fi
if test -z "$ZIP";       then AC_SUBST([ZIP],       [:]) fi

if test -z "$MAKEINFO";  then AC_SUBST([MAKEINFO],  [:]) fi
if test -z "$DOXYGEN";   then AC_SUBST([DOXYGEN],   [:]) fi
if test -z "$CHMOD";     then AC_SUBST([CHMOD],     [:]) fi
if test -z "$LATEX";     then AC_SUBST([LATEX],     [:]) fi
if test -z "$MAKEINDEX"; then AC_SUBST([MAKEINDEX], [:]) fi

# check libraries
#   libxml2
AC_SUBST([LIBXML_CFLAGS], [])
if test -n "$PKGCONFIG"; then
    if test `$PKGCONFIG --exists libxml-2.0; echo $?` -eq 0; then
        AC_SUBST([LIBXML_CFLAGS], [`\${PKGCONFIG} libxml-2.0 --cflags`])
        AC_SUBST([LIBXML_LIBS], [`\${PKGCONFIG} libxml-2.0 --libs`])
    fi
fi
if test -z "$LIBXML_LIBS"; then
    AC_SUBST([LIBXML_LIBS], [-lxml2])
    if test -d /usr/include/libxml2/libxml; then
        AC_SUBST([LIBXML_CFLAGS], [-I/usr/include/libxml2])
    elif test -d /usr/local/include/libxml2/libxml; then
        AC_SUBST([LIBXML_CFLAGS], [-I/usr/local/include/libxml2])
    elif test -d "$prefix/usr/include/libxml2/libxml"; then
        AC_SUBST([LIBXML_CFLAGS], [-I"$prefix/usr/include/libxml2"])
    elif test -d "$prefix/usr/local/include/libxml2/libxml"; then
        AC_SUBST([LIBXML_CFLAGS], [-I"$prefix/usr/local/include/libxml2"])
    fi
fi
CXXFLAGS="$CXXFLAGS_ORI $LIBXML_CFLAGS"
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

CPPFLAGS="$CPPFLAGS_ORI $LIBXML_CFLAGS"
CXXFLAGS="$CXXFLAGS_ORI $LIBXML_CFLAGS"
AC_CHECK_HEADER([libxml/xmlreader.h],, AC_MSG_ERROR([Failed to find <xmlreader.h>]))

# check types
AS_IF([test "x$enable_fraction_check" = "xyes"],
    AC_TYPE_LONG_LONG_INT)
AC_TYPE_SIZE_T

# check functions
CPPFLAGS="$CPPFLAGS_ORI $LIBXML_CFLAGS"
CXXFLAGS="$CXXFLAGS_ORI $LIBXML_CFLAGS"
LDFLAGS="$LDFLAGS_ORI $LIBXML_LIBS"
LIBS="$LIBS_ORI $LIBXML_LIBS"
AC_CHECK_FUNC([xmlReaderForFile],, AC_MSG_ERROR([library 'libxml2' does not provide the 'xmlReader' interface]))
AC_CHECK_FUNC([xmlReaderForMemory],, AC_MSG_ERROR([library 'libxml2' does not provide the 'xmlReader' interface]))

# create rpath
if test -n "$RPATHFLAG"; then
    buffer=$LIBXML_LIBS
    for path in $buffer $LIBXML; do
        path=$(expr "$path" : "-L\(.*\)")
        if test -n "$path"; then
            LIBXML_LIBS="$LIBXML_LIBS $RPATHFLAG$path"
        fi
    done
    
    if test "x$RPATH" != "x"; then
        buffer=
        for path in $RPATH; do
            buffer="$buffer $RPATHFLAG$path"
        done
        RPATH=$buffer
    fi
    
    for path in $LDFLAGS; do
        path=$(expr "$path" : "-L\(.*\)")
        if test -n "$path"; then
            RPATH="$RPATH $RPATHFLAG$path"
        fi
    done
    AC_SUBST([RPATH], [$RPATH])
fi

# generate output
AC_SUBST([CPPFLAGS],[$CPPFLAGS_ORI])
AC_SUBST([CXXFLAGS],[$CXXFLAGS_ORI])
AC_SUBST([LDFLAGS],[$LDFLAGS_ORI])
AC_SUBST([LIBS],[$LIBS_ORI])
AC_OUTPUT()

