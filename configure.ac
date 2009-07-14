# Copyright IBM Corp. 2007
AC_INIT(CMPI Utility Library, 0.5.1, kaitlin@us.ibm.com, libcmpiutil)

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

AM_INIT_AUTOMAKE

AC_PROG_CC

AC_PROG_LEX
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

