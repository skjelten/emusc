#!/bin/bash
# Shell script for deploying QT and extracting all other relevant DLL files on
# a Windows system running in a MSYS2 environment.
# Part of the EmuSC project: https://github.com/skjelten/emusc

DEPLOYCMD=""

if command -v windeployqt.exe &> /dev/null
then
    DEPLOYCMD="windeployqt.exe"
fi

if [ -z $DEPLOYCMD ]
then
    if command -v windeployqt6.exe &> /dev/null
    then
	DEPLOYCMD="windeployqt6.exe"
    else
	echo "Error: windeployqt.exe / windeployqt6.exe is not in PATH. Install the Qt tools package, e.g. 'pacman -S mingw-w64-ucrt-x86_64-qt6-tools' if you are using Qt6."
	exit 1
    fi
fi

if ! command -v ldd &> /dev/null
then
	echo "Error: ldd is not in PATH. Install GCC or fix your PATH variable."
	exit 1
fi

if [ $# -ne 2 ]; then
	echo "Usage: msys_copy_all_dlls.sh SRC_DIRECTORY_WITH_EMUSC.EXE DEST_DIRECTORY"
	exit 1
fi

if [ ! -d "$1" ]; then
	echo "Error: $1 is not a valid directory"
	exit 1
fi

if [ ! -f "$1/emusc.exe" ]; then
	echo "Error: $1 is not a valid directory (missing emusc.exe)"
	exit 1
fi

if [ ! -d "$2" ]; then
	mkdir $2
	if [ ! -d "$2" ]; then
	    echo "Error: The directory $2 does not exist and unable create it."
	    exit 1
	fi
fi

# 1. Deply QT

echo "$DEPLOYCMD --no-translations --dir $2 --no-compiler-runtime --no-translations --verbose 1 $1/emusc.exe" | bash

# 2. Copy libemusc.dll
cp $1/../../libemusc/src/libemusc.dll $2

# 3. Find and copy all other DLL-files needed to run emusc.exe
#    Note that the ldd command must to be run in the folder with all the DLLs
cd $2
ldd $1/emusc.exe | grep -iv system32 | grep -iv windows | grep -iv qt5 | grep -iv qt6 | grep -iv "not found" | cut -f2 -d\> | cut -f1 -d\( | tr '\\' / | while read a; do cp "$a" "$2"; done
