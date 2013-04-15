# Copyright IBM Corp. 2007
m4_define([cmpiutil_maj], [0])
m4_define([cmpiutil_min], [5])
m4_define([cmpiutil_mic], [7])
m4_define([cmpiutil_version], [cmpiutil_maj.cmpiutil_min.cmpiutil_mic])

AC_INIT([CMPI Utility Library], [cmpiutil_version], [cvincent@us.ibm.com], [libcmpiutil])

AC_CONFIG_SRCDIR([libcmpiutil.h])

CHECK_CMPI
CHECK_IND_VOID

AC_CHECK_HEADERS([cmpidt.h cmpift.h cmpimacs.h])

CC_WARNINGS="\
-Wall \
-Wmissing-prototypes \
-Wmissing-declarations \
-Wstrict-prototypes \
-Wpointer-arith \
-Wformat=2 \
-Wformat-security \
-Wformat-nonliteral \
-Wno-format-y2k \
-Wcast-align \
-Wno-unused-value"

CFLAGS="$CFLAGS $CC_WARNINGS -D_FORTIFY_SOURCE=2 -D_GNU_SOURCE"

AC_CONFIG_HEADER([config.h])

topdir=`pwd`
AC_SUBST(topdir)

AC_CONFIG_FILES([Makefile tools/Makefile])

# Use silent-rules if possible
AM_INIT_AUTOMAKE
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

m4_define([lt_cur], m4_eval(cmpiutil_maj + cmpiutil_min))
m4_define([lt_rev], cmpiutil_mic)
m4_define([lt_age], cmpiutil_min)
CMPIUTIL_VERSION_INFO="lt_cur:lt_rev:lt_age"
AC_SUBST(CMPIUTIL_VERSION_INFO)

AC_PROG_CC

AC_PROG_LEX
AC_SUBST([LEX_OUTPUT_ROOT], [lex.eo_parse_])
AC_PROG_YACC

AC_C_CONST
AC_PROG_LIBTOOL
AC_PROG_INSTALL

AC_CHECK_PROG([found_doxygen],[doxygen],[yes])
AM_CONDITIONAL([found_doxygen], [test x$found_doxygen = xyes])

AC_ARG_ENABLE([eoparser],
	[  --enable-eoparser          Build MOF Embedded Object Parser support],
	[eoparser=${enableval}],
	[eoparser=yes])

AM_CONDITIONAL([build_eoparser],[test x$eoparser = xyes])

# Check for pkg-config program
PKG_PROG_PKG_CONFIG
CHECK_LIBXML2

if test x${eoparser} = xyes; then
   CFLAGS+=" -DHAVE_EOPARSER"
fi

CFLAGS_STRICT="-Werror"
AC_SUBST(CFLAGS_STRICT)

CHECK_BROKEN_CMPIFT

echo ""
echo "Build Embedded Object parser:     $eoparser"
echo ""

AC_OUTPUT(libcmpiutil.spec libcmpiutil.pc)

