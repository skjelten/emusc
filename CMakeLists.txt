# CMake & Cpack definitions for EmuSC project

# The project consists of a total of 5 CMakeList.txt files. This root file is
# primarily used for convinience (build both GUI client and library) and for
# Cpack generators covering both client and library.

cmake_minimum_required(VERSION 3.12...3.30)

project(
  emusc
  VERSION 0.3.0
  HOMEPAGE_URL "https://github.com/skjelten/emusc"
  LANGUAGES CXX)

option(emusc_WITH_EMUSC_CLIENT "Build GUI client application" TRUE)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(libemusc)

if (emusc_WITH_EMUSC_CLIENT)
  add_subdirectory(emusc)
  add_dependencies(emusc-client emusc)
endif()

# CPack support
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Roland SC-55 synth emulator")
set(CPACK_PACKAGE_VENDOR "skjelten.org")
set(CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/emusc/res/icons/32x32/emusc.png")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "EmuSC")
set(CPACK_PACKAGE_EXECUTABLES "emusc" "SC-55 synth emulator")
set(CPACK_PACKAGE_CONTACT "skjelten@pvv.org")

set(CPACK_PACKAGE_VERSION_MAJOR ${CMAKE_PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${CMAKE_PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${CMAKE_PROJECT_VERSION_PATCH})

set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/emusc/COPYING")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

# Source packages
set(CPACK_SOURCE_GENERATOR TGZ)
set(CPACK_SOURCE_IGNORE_FILES "${CPACK_SOURCE_IGNORE_FILES}")
# use .gitognore?
set(CPACK_SOURCE_PACKAGE_FILE_NAME "emusc-${PROJECT_VERSION}")
set(CPACK_SOURCE_STRIP_FILES ON)

# DEB and RPM specifc variables
if(UNIX AND NOT APPLE)
  set(CPACK_COMPONENTS_GROUPING IGNORE)

  set(CPACK_DEB_COMPONENT_INSTALL ON)
  set(CPACK_DEBIAN_ENABLE_COMPONENT_DEPENDS ON)

  # Note: This hack only works for CMAKE >= 3.20
  #       On CMAKE < 3.20 uncomment RPATH stuff in emusc/CMakeList.txt
  set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS_PRIVATE_DIRS "../lib/usr/lib")

  # DEB common settings for all components
  set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Håkon Skjelten <${CPACK_PACKAGE_CONTACT}>")
  set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/skjelten/emusc")
  set(CPACK_DEBIAN_PACKAGE_SECTION "sound")
  set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
  set(CPACK_DEBIAN_PACKAGE_GENERATE_SHLIBS ON)
  set(CPACK_DEBIAN_PACKAGE_GENERATE_SHLIBS_POLICY ">=")

  # DEB component specific settings
  set(CPACK_DEBIAN_BIN_PACKAGE_NAME "emusc")
  set(CPACK_DEBIAN_BIN_FILE_NAME DEB-DEFAULT)
  set(CPACK_DEBIAN_BIN_PACKAGE_DEPENDS "libemusc (>= ${PROJECT_VERSION})")

  set(CPACK_DEBIAN_LIB_PACKAGE_NAME "libemusc")
  set(CPACK_DEBIAN_LIB_FILE_NAME DEB-DEFAULT)

  set(CPACK_DEBIAN_DEV_PACKAGE_NAME "libemusc-dev")
  set(CPACK_DEBIAN_DEV_FILE_NAME DEB-DEFAULT)
  set(CPACK_DEBIAN_DEV_PACKAGE_DEPENDS "pkgconf")

  # RPM common settings for all components
  set(CPACK_RPM_COMPONENT_INSTALL ON)
  set(CPACK_RPM_PACKAGE_LICENSE "GPLv3+")

  # RPM component specific settings
  set(CPACK_RPM_BIN_PACKAGE_NAME "emusc")
  set(CPACK_RPM_BIN_FILE_NAME RPM-DEFAULT)
  set(CPACK_RPM_BIN_PACKAGE_DEPENDS "libemusc (>= ${PROJECT_VERSION})")

  set(CPACK_RPM_LIB_PACKAGE_NAME "libemusc")
  set(CPACK_RPM_LIB_FILE_NAME RPM-DEFAULT)

  set(CPACK_RPM_DEV_PACKAGE_NAME "libemusc-devel")
  set(CPACK_RPM_DEV_FILE_NAME RPM-DEFAULT)
  set(CPACK_RPM_DEV_PACKAGE_DEPENDS "pkgconf")
  set(CPACK_RPM_DEV_PACKAGE_ARCHITECTURE "noarch")
  
# Windows (NSIS) specific variables
elseif(WIN32)
  set(CPACK_NSIS_PACKAGE_NAME "EmuSC")
  set(CPACK_NSIS_DISPLAY_NAME "EmuSC ${CMAKE_PROJECT_VERSION}")
  set(CPACK_NSIS_HELP_LINK "https://github.com/skjelten/emusc/wiki")
  set(CPACK_NSIS_URL_INFO_ABOUT "https://github.com/skjelten/emusc")
  set(CPACK_NSIS_CONTACT "https://github.com/skjelten/emusc")
  set(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/emusc/res/icons\\\\emusc.ico")
  set(CPACK_NSIS_MUI_WELCOMEFINISHPAGE_BITMAP "${CMAKE_CURRENT_SOURCE_DIR}/emusc/res/images/installers\\\\nsis_welcome.bmp")
  set(CPACK_NSIS_MUI_HEADERIMAGE "${CMAKE_CURRENT_SOURCE_DIR}/emusc/res/images/installers\\\\nsis_header.bmp")
  set(CPACK_NSIS_MODIFY_PATH OFF)
  set(CPACK_NSIS_BRANDING_TEXT "EmuSC v${CMAKE_PROJECT_VERSION}")

elseif(APPLE)
  set(CPACK_DMG_VOLUME_NAME "EmuSC")
  set(CPACK_DMG_BACKGROUND_IMAGE "${CMAKE_CURRENT_SOURCE_DIR}/emusc/res/images/installers/dmg_background.png")

endif()

include(CPack)

cpack_add_component(bin
  DISPLAY_NAME "EmuSC"
  DESCRIPTION "Application for reading MIDI input and generating SC-55 audio PCM"
  INSTALL_TYPES "User"
  GROUP Runtime)
cpack_add_component(lib
  DISPLAY_NAME "EmuSC Library"
  DESCRIPTION "Library used to build and run programs generating SC-55 audio (libemusc)"
  DISABLED
  INSTALL_TYPES "Developer"
  GROUP Development)
cpack_add_component(dev
  DISPLAY_NAME "C++ Headers"
  DESCRIPTION "C++ header files needed for developers to create new software using the EmuSC Library (libemusc)"
  DISABLED
  INSTALL_TYPES "Developer"
  GROUP Development
  DEPENDS lib)

cpack_add_component_group(Runtime
  DISPLAY_NAME "Runtime"
  DESCRIPTION "End user software for emulating the SoundCanvas"
  EXPANDED)

cpack_add_component_group(Development
  DISPLAY_NAME "Development"
  DESCRIPTION "Development files for creating new applications based on EmuSC (libemusc)"
  EXPANDED)
