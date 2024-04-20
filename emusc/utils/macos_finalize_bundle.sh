#!/bin/sh

APP_NAME="EmuSC"
APP_DIR="${APP_NAME}.app"

if [ $# -ne 1 ]; then
    echo "Usage: macos_finalize_bundle.sh PATH_TO_EMUSC_SRC_DIRECTORY"
    exit
fi

if [ ! -d "${1}/${APP_DIR}" ]
then
    echo "Error: EmuSC bundle (${APP_DIR}) not found in src directory (${1})"
    exit
fi

if ! command -v macdeployqt &> /dev/null
then
    echo "Error: macdeplyqt is not in PATH."
    exit
fi

# First deplay all of QT
macdeployqt ${1}/${APP_DIR}

# Copy libEmuSC to the bundle
# Note that creating the Framwork directory and setting rpath to this
# directory is done by the macdeplyqt script above
cp -v ${1}/../../libemusc/src/libemusc.dylib ${1}/${APP_DIR}/Contents/Frameworks

echo ""
echo "${APP_DIR} is now ready to be deployed."
echo "You can use the macos_create_dmg.sh script to create a dmg install file."
echo ""
