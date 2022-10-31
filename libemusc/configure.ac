dnl Process this file with autoconf to produce a configure script.

AC_INIT([libEmuSC],[0.0.1],[skjelten@pvv.org],[libemusc],
	[https://github.com/skjelten/emusc])
AC_CONFIG_SRCDIR([src/synth.cc])

AC_CANONICAL_HOST
AC_CANONICAL_TARGET

AC_CONFIG_MACRO_DIRS([m4])
AC_CONFIG_HEADERS(config.h)

AM_INIT_AUTOMAKE

LT_INIT

AC_PROG_CXX

dnl ABI version: current : revision : age. Rule for updating:
dnl - Always increase the revision value
dnl - Increase the current value whenever an interface has been added, removed or changed
dnl - Increase the age value only if the changes made to the ABI are backward compatible
AC_SUBST([LIBEMUSC_SO_VERSION], [0:1:0])

dnl API version: Follows normal versioning scheme and follows package version
AC_SUBST([LIBEMUSC_API_VERSION], [0.0.1])


dnl   Setting some architecture dependent compiler options
dnl -======================================================-
case "$host" in
     	*-*-mingw32*)
		;;
	*-*-darwin*)
		;;
	*)
		;;
esac


dnl Generating Makefiles:
AC_CONFIG_FILES([
Makefile
src/Makefile
])
AC_OUTPUT


echo ""
echo ""
echo " libEmuSC installation summary:"
echo "-==============================-"
echo "Host platform: "${host}
echo "Version: $PACKAGE_VERSION"
echo ""

echo ""
echo "Have fun!"
echo ""
