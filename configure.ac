
### Version

m4_define([v_maj], [0])
m4_define([v_min], [0])
m4_define([v_mic], [9])
m4_define([v_ver], [v_maj.v_min.v_mic])

m4_define([lt_cur], m4_eval(v_maj + v_min))
m4_define([lt_rev], v_mic)
m4_define([lt_age], v_min)

AC_INIT([eguebfs], [v_ver], [enesim-devel@googlegroups.com])
AC_PREREQ([2.60])
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([foreign 1.6 dist-bzip2])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

LT_PREREQ([2.2])
LT_INIT([win32-dll])

version_info="lt_cur:lt_rev:lt_age"
AC_SUBST([version_info])

AC_DEFINE_UNQUOTED([VERSION_MAJOR], [v_maj], [Major version])
AC_DEFINE_UNQUOTED([VERSION_MINOR], [v_min], [Minor version])
AC_DEFINE_UNQUOTED([VERSION_MICRO], [v_mic], [Micro version])

VMAJ=v_maj
AC_SUBST([VMAJ])

## Add parameters for aclocal (keep in sync with Makefile.am)
AC_SUBST(ACLOCAL_AMFLAGS, "-I m4 -I common/m4")

### default values

AC_CANONICAL_HOST

have_win32="no"
case "$host_os" in
   mingw*)
      have_win32="yes"
   ;;
esac

AM_CONDITIONAL([HAVE_WIN32], [test "x${have_win32}" = "xyes"])

### Checks for programs

AC_PROG_CC
AM_PROG_CC_C_O

AC_SYS_LARGEFILE

# pkg-config
PKG_PROG_PKG_CONFIG

# Check whether pkg-config supports Requires.private
if ! $PKG_CONFIG --atleast-pkgconfig-version 0.22; then
   AC_MSG_ERROR([pkg-config >= 0.22 is required])
fi

# doxygen program for documentation building
ENS_CHECK_DOXYGEN([build_doc="yes"], [build_doc="no"])

LT_SYS_MODULE_EXT

### Checks for libraries
requirements_pc="eina egueb-dom"
requirements_private_pc="fuse"
PKG_CHECK_MODULES([EGUEBFS], [${requirements_pc} ${requirements_private_pc}])

requirements_bin_pc="eina egueb-dom efl-egueb ecore"
PKG_CHECK_MODULES([EGUEBFS_BIN], [${requirements_bin_pc}])

ENS_CHECK_COMPILER_FLAGS([EGUEBFS], [-Wall])

AC_SUBST([requirements_pc])
AC_SUBST([requirements_private_pc])

### Checks for library functions

# Unit tests, coverage and benchmarking

ENS_CHECK_TESTS([enable_tests="yes"], [enable_tests="no"])

## Make the debug preprocessor configurable

AC_CONFIG_FILES([
Makefile
common/doc/Doxyfile_common
])

AC_OUTPUT

#####################################################################
## Info

echo
echo
echo
echo "------------------------------------------------------------------------"
echo "$PACKAGE $VERSION"
echo "------------------------------------------------------------------------"
echo
echo
echo "Configuration Options Summary:"
echo
echo "  Build tests.............................: ${enable_tests}"
echo
echo "Compilation...............................: make (or gmake)"
echo "  CPPFLAGS................................: $CPPFLAGS"
echo "  CFLAGS..................................: $CFLAGS"
echo "  LDFLAGS.................................: $LDFLAGS"
echo
echo "Installation..............................: make install (as root if needed, with 'su' or 'sudo')"
echo "  prefix..................................: $prefix"
echo
