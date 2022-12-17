#!/bin/sh

if ! command -v windeployqt.exe &> /dev/null
then
	echo "Error: windeployqt.exe is not in PATH. Install Qt5 tools, e.g. 'pacman -S mingw-w64-ucrt-x86_64-qt5-tools'"
	exit
fi

if ! command -v ldd &> /dev/null
then
	echo "Error: ldd is not in PATH. Install GCC or fix your PATH variable"
	exit
fi

if [ $# -ne 1 ]; then
	echo "Usage: msys_create_dll_bundle.sh PATH_TO_EMUSC_SRC_DIRECTORY"
	exit
fi

if [ ! -f "$1/emusc.exe" ]; then
	echo "Error: emusc.exe not found in $1. Wrong directory og build incomplete."
	exit
fi

DESTDIR="$1/../BUILD_WIN32"

# 1. Make build directory and copy emusc.exe
mkdir -v $DESTDIR
cp $1/emusc.exe $DESTDIR

# 2. Deply QT
windeployqt.exe --no-translations $DESTDIR/emusc.exe

# 3. Copy libemusc.dll
cp -v $1/../../libemusc/src/libemusc.dll $DESTDIR

# 4. Find and copy all other DLL-files needed to run emusc.exe
ldd $DESTDIR/emusc.exe | grep -iv system32 | grep -iv windows | grep -iv qt5 | grep -iv "not found" | grep -iv libemusc | cut -f2 -d\> | cut -f1 -d\( | tr \\ / | while read a; do cp -v "$a" "$DESTDIR"; done

