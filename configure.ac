dnl Process this file with autoconf to produce a configure script.

AC_INIT([emusc],[0.01])
AC_CONFIG_SRCDIR([src/emusc.cc])

AC_CANONICAL_HOST
AC_CANONICAL_TARGET

AC_CONFIG_HEADERS(config.h)

AM_INIT_AUTOMAKE

dnl Tools
AC_PROG_CXX

AC_MSG_CHECKING(for audio API)
echo ""
useAlsa="no"
useCore="no"
usePulse="no"
useWin32="no"

dnl   Check for ALSA
dnl -================-
AC_CHECK_LIB(asound, snd_seq_open,
             LIBS="-lasound -lpthread $LIBS"
	     CXXFLAGS="$CXXFLAGS -D__ALSA__"
	     useAlsa="yes",
	     AC_MSG_WARN(ALSA library not found and will not be supported))

dnl   Check for PulseAudio
dnl -======================-
AC_CHECK_LIB(pulse, main,
             LIBS="-lpulse $LIBS"
	     CXXFLAGS="$CXXFLAGS -D__PULSE__"
	     usePulse="yes",
	     AC_MSG_WARN(PulseAudio library not found and will not be supported))

dnl   Check for Winmm
dnl -=================-
AC_CHECK_LIB(winmm, main,
	     LIBS="-lwinmm $LIBS"
	     CXXFLAGS="$CXXFLAGS -D__WIN32__"
	     useWin32="yes",
	     AC_MSG_WARN(Winmm library not found and will not be supported))

dnl   Check for Core Audio / MIDI   FIXME->Proper test for audio/MIDI framework
dnl -=============================-
AC_CHECK_HEADER(CoreAudio/CoreAudio.h,
	     LIBS="-framework CoreMIDI -framework CoreFoundation -framework CoreServices -framework AudioUnit $LIBS"
	     CXXFLAGS="$CXXFLAGS -D__CORE__"
	     useCore="yes",
	     AC_MSG_WARN(CoreAudio library not found and will not be supported))


dnl   Setting some architecture dependent compiler options
dnl -======================================================-
case "$host" in
     	*-*-mingw32*)
		LDFLAGS="$LDFLAGS -static-libstdc++ -static"
		CXX=i686-w64-mingw32-g++-posix
		;;
	*-*-darwin*)
dnl		CXXFLAGS="$CXXFLAGS -O3 -mdynamic-no-pic"		
		useCore="yes"
		;;
	*)
dnl		CXXFLAGS="$CXXFLAGS -O3"
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
echo "  EmuSC installation summary:"
echo "-=============================-"
echo "Host platform: "${host}
echo ""
echo "MIDI APIs:"
echo "  * ALSA  : `eval echo ${useAlsa}`"
echo "  * Core  : `eval echo ${useCore}`"
echo "  * Win32 : `eval echo ${useWin32}`"
echo ""
echo "Audio APIs:"
echo "  * ALSA  : `eval echo ${useAlsa}`"
echo "  * Core  : `eval echo ${useCore}`"
echo "  * Null  : yes"
echo "  * Pulse : `eval echo ${usePulse}`"
echo "  * Win32 : `eval echo ${useWin32}`"

echo ""
echo "Have fun!"
echo ""
