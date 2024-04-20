#!/bin/sh

APP_NAME="EmuSC"
APP_DIR="${APP_NAME}.app"
DMG_FILE_NAME="${APP_NAME}-Installer.dmg"
VOLUME_NAME="${APP_NAME} Installer"
DEST_DIR="${1}/.."

if [ $# -ne 1 ]; then
    echo "Usage: macos_create_dmg.sh PATH_TO_EMUSC_SRC_DIRECTORY"
    exit
fi

if [ ! -d "${1}/${APP_DIR}" ]
then
    echo "Error: EmuSC bundle (${APP_DIR}) not found in src directory (${1})"
    exit
fi

if ! command -v create-dmg &> /dev/null
then
    echo "Error: create-dmg is not in PATH. Install with homebrew "
    echo "'brew install create-dmg' or download from "
    echo "https://github.com/create-dmg/create-dmg"
    exit
fi

test -f ${DEST_DIR}/${DMG_FILE_NAME} && rm ${DEST_DIR}/${DMG_FILE_NAME}

create-dmg \
  --volname "${VOLUME_NAME}" \
  --background "${DEST_DIR}/res/images/macos_dmg_bkg.png" \
  --window-pos 200 120 \
  --window-size 800 400 \
  --icon-size 100 \
  --icon "${APP_DIR}" 200 190 \
  --hide-extension "${APP_DIR}" \
  --app-drop-link 600 185 \
  "${DEST_DIR}/${DMG_FILE_NAME}" \
  "${1}/${APP_DIR}"
