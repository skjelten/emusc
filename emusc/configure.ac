dnl Process this file with autoconf to produce a configure script.

AC_INIT([emusc],[0.01])
AC_CONFIG_SRCDIR([src/emusc.cc])

AC_CANONICAL_HOST
AC_CANONICAL_TARGET

AC_CONFIG_HEADERS(config.h)

AM_INIT_AUTOMAKE

dnl Tools
AC_PROG_CXX
AX_CXX_COMPILE_STDCXX([11], [noext], [mandatory])

dnl   Check for libEmuSC
dnl -====================-
AC_CHECK_LIB(emusc, main,
	LIBS="$LIBS -lemusc"
	,
	AC_MSG_ERROR(libEmuSC library is required but not found!))

dnl   Check for libQt5 and tools
dnl -============================-
dnl Note: Using PKG_CHECK_MODULES on libQt to find correct paths
QT_REQUIRED=5.7.0
PKG_CHECK_MODULES([QT],[Qt5Core >= $QT_REQUIRED Qt5Gui >= $QT_REQUIRED Qt5Widgets >= $QT_REQUIRED],
		  [CXXFLAGS="$CXXFLAGS $QT_CFLAGS -fPIC"]
		  [LIBS="$LIBS $QT_LIBS"]
		  ,
		  [AC_MSG_ERROR(Valid Qt library is required but not found!)])

AC_PATH_PROGS([QT_MOC], [moc, moc-qt5], [no], ["${QT_BIN_DIRECTORY}:${PATH}"])
if test x${QT_MOC} = xno ; then
   AC_MSG_ERROR([Qt 'moc' tool is required but not found. Please add correct path to program in QT_BIN_DIRECTORY variable])
fi
AC_SUBST([QT_MOC])

AC_PATH_PROGS([QT_RCC], [rcc, rcc-qt5], [no], ["${QT_BIN_DIRECTORY}:${PATH}"])
if test x${QT_RCC} = xno ; then
   AC_MSG_ERROR([Qt 'rcc' tool is required but not found. Please add correct path to program in QT_BIN_DIRECTORY variable])
fi
AC_SUBST([QT_RCC]) 


dnl Checking for audio and MIDI libraries
useAlsaAudio="no"
useCoreAudio="no"
usePulseAudio="no"
useQtAudio="no"
useWin32Audio="no"

useAlsaMIDI="no"
useCoreMIDI="no"
useWin32MIDI="no"

dnl   Check for ALSA
dnl -================-
AC_CHECK_LIB(asound, snd_seq_open,
             LIBS="-lasound -lpthread $LIBS"
	     CXXFLAGS="$CXXFLAGS -D__ALSA_AUDIO__ -D__ALSA_MIDI__"
	     useAlsaAudio="yes"
	     useAlsaMIDI="yes",
	     AC_MSG_WARN(ALSA library not found and will not be supported))

dnl   Check for PulseAudio
dnl -======================-
AC_CHECK_LIB(pulse, main,
             LIBS="-lpulse $LIBS"
	     CXXFLAGS="$CXXFLAGS -D__PULSE_AUDIO__"
	     usePulseAudio="yes",
	     AC_MSG_WARN(PulseAudio library not found and will not be supported))

dnl   Check for QtMultimedia
dnl -========================-
dnl Note: Using PKG_CHECK_MODULES on libQt to find correct paths
PKG_CHECK_MODULES([QTMULTIMEDIA], [Qt5Multimedia >= $QT_REQUIRED],
		  [CXXFLAGS="$CXXFLAGS $QTMULTIMEDIA_CFLAGS"
		  LIBS="$LIBS $QTMULTIMEDIA_LIBS"]
		  [useQtAudio="yes"],
		  [AC_MSG_WARN(Qt5 Multimedia library not found and will not be supported)])

dnl   Check for Winmm
dnl -=================-
AC_CHECK_LIB(winmm, main,
	     LIBS="-lwinmm $LIBS"
	     CXXFLAGS="$CXXFLAGS -D__WIN32_AUDIO__ -D__WIN32_MIDI__"
	     useWin32Audio="yes"
	     useWin32MIDI="yes",
	     AC_MSG_WARN(Winmm library not found and will not be supported))

dnl   Check for Core Audio / MIDI   FIXME->Proper test for audio/MIDI framework
dnl -=============================-
AC_CHECK_HEADER(CoreAudio/CoreAudio.h,
	     LIBS="-framework CoreAudio -framework CoreMIDI -framework CoreFoundation -framework CoreServices -framework AudioUnit $LIBS"
	     CXXFLAGS="$CXXFLAGS -D__CORE_AUDIO__ -D__CORE_MIDI__"
	     useCoreAudio="yes"
	     useCoreMIDI="yes",
	     AC_MSG_WARN(CoreAudio library not found and will not be supported))


dnl   Setting some architecture dependent compiler options
dnl -======================================================-
case "$host" in
     	*-*-mingw32*)
		LDFLAGS="$LDFLAGS -static-libstdc++"
		;;
	*-*-darwin*)
dnl		CXXFLAGS="$CXXFLAGS -O3 -mdynamic-no-pic"		
		;;
	*)
		;;
esac


dnl Generating Makefiles:
AC_CONFIG_FILES([
Makefile
src/Makefile
resources/Makefile
])
AC_OUTPUT


echo ""
echo ""
echo "  EmuSC installation summary:"
echo "-=============================-"
echo "Host platform: "${host}
echo ""
echo "MIDI APIs:"
echo "  * ALSA  : `eval echo ${useAlsaMIDI}`"
echo "  * Core  : `eval echo ${useCoreMIDI}`"
echo "  * Win32 : `eval echo ${useWin32MIDI}`"
echo ""
echo "Audio APIs:"
echo "  * ALSA  : `eval echo ${useAlsaAudio}`"
echo "  * Core  : `eval echo ${useCoreAudio}`"
echo "  * Null  : yes"
echo "  * Pulse : `eval echo ${usePulseAudio}`"
echo "  * Qt    : `eval echo ${useQtAudio}`"
echo "  * Win32 : `eval echo ${useWin32Audio}`"

echo ""
echo "Have fun!"
echo ""
