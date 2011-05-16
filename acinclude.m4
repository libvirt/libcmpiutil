# Copyright IBM Corp. 2007
dnl
dnl Helper functions
dnl
AC_DEFUN([_CHECK_CMPI],
        [
        AC_MSG_CHECKING($1)
        AC_TRY_LINK(
        [
                #include <cmpimacs.h>
                #include <cmpidt.h>
                #include <cmpift.h>
        ],
        [
                CMPIBroker broker;
                CMPIStatus status = {CMPI_RC_OK, NULL};
                CMPIString *s = CMNewString(&broker, "TEST", &status);
        ],
        [
                have_CMPI=yes
                dnl AC_MSG_RESULT(yes)
        ],
        [
                have_CMPI=no
                dnl AC_MSG_RESULT(no)
        ])

])

dnl
dnl The main function to check for CMPI headers
dnl Modifies the CPPFLAGS with the right include directory and sets
dnl the 'have_CMPI' to either 'no' or 'yes'
dnl
AC_DEFUN([CHECK_CMPI],
        [
        AC_MSG_CHECKING(for CMPI headers)
        dnl Check just with the standard include paths
        CMPI_CPP_FLAGS="$CPPFLAGS"
        _CHECK_CMPI(standard)
        if test "$have_CMPI" == "yes"; then
                dnl The standard include paths worked.
                AC_MSG_RESULT(yes)
        else
                AC_MSG_RESULT(no)
          _DIRS_="/usr/include/cmpi \
                  /usr/local/include/cmpi \
                  $PEGASUS_ROOT/src/Pegasus/Provider/CMPI \
                  /opt/tog-pegasus/include/Pegasus/Provider/CMPI \
                  /usr/include/Pegasus/Provider/CMPI \
                  /usr/include/openwbem \
                  /usr/sniacimom/include"
          for _DIR_ in $_DIRS_
          do
                 _cppflags=$CPPFLAGS
                 _include_CMPI="$_DIR_"
                 CPPFLAGS="$CPPFLAGS -I$_include_CMPI"
                 _CHECK_CMPI($_DIR_)
                 if test "$have_CMPI" == "yes"; then
                        dnl Found it
                        AC_MSG_RESULT(yes)
                        dnl Save the new -I parameter
                        CMPI_CPP_FLAGS="$CPPFLAGS"
                        break
                 else
                        AC_MSG_RESULT(no)
		 fi
                 CPPFLAGS=$_cppflags
          done
        fi
        CPPFLAGS="$CMPI_CPP_FLAGS"
        if test "$have_CMPI" == "no"; then
		AC_MSG_ERROR(Cannot find CMPI header files.)
        fi
        ]
)

AC_DEFUN([CHECK_BROKEN_CMPIFT],
	[
	AC_MSG_CHECKING(for a broken cmpift.h)
	AC_RUN_IFELSE([
	#include <cmpift.h>

	int main(void) {
	  if ((sizeof(unsigned long) == 8) &&
	      (sizeof(struct _CMPIBrokerFT) != 160))
	    return 1;
	  else
	    return 0;
	}
	],[
	    AC_MSG_RESULT(no)
	],[
	    AC_MSG_ERROR(You have a broken cmpift.h header.  Is a broken version of sblim-cmpi-devel installed?)
	]
)])

#
# Check for void EnableIndications return
#
AC_DEFUN([CHECK_IND_VOID], [
	AH_TEMPLATE([CMPI_EI_VOID],
		    [Defined if return type of EnableIndications
		     should be void])
	AC_MSG_CHECKING([return type for indications])
	CFLAGS_TMP=$CFLAGS
	CFLAGS="-Werror"
	AC_TRY_COMPILE([
		  #include <cmpift.h>
		  static void ei(CMPIIndicationMI *mi, const CMPIContext *c) {
		       return;
		  }
		],[
		  struct _CMPIIndicationMIFT ft;
		  ft.enableIndications = ei;
		  return 0;
	], [
		echo "void"
		AC_DEFINE_UNQUOTED([CMPI_EI_VOID], [yes])
	], [
		echo "CMPIStatus"
	])
	CFLAGS=$CFLAGS_TMP
])

AC_DEFUN([CHECK_LIBXML2],
        [
        PKG_CHECK_MODULES([LIBXML], [libxml-2.0])
        CPPFLAGS="$CPPFLAGS $LIBXML_CFLAGS"
        LDFLAGS="$LDFLAGS $LIBXML_LDFLAGS"
        ])
